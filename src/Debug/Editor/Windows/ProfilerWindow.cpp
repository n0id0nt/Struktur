#include "ProfilerWindow.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "Engine/Core/GameData.h"
#include "Engine/Core/TimeSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/GraphicsDevice.h"

// Shared phase palette - used by the graph bands, the stacked summary bar and the tree headers so the same
// phase always reads as the same colour everywhere in the window.
static const ImU32 kColFixed     = IM_COL32(232, 151, 58, 255);
static const ImU32 kColUpdate    = IM_COL32(80, 150, 232, 255);
static const ImU32 kColRender    = IM_COL32(92, 200, 122, 255);
static const ImU32 kColOther     = IM_COL32(150, 150, 160, 255);
static const ImU32 kColFrameLine = IM_COL32(240, 240, 245, 255);
static const ImU32 kColGuide     = IM_COL32(255, 255, 255, 38);
static const ImU32 kColGuideText = IM_COL32(200, 200, 210, 150);
static const ImU32 kColGraphBg   = IM_COL32(18, 18, 22, 255);

// The four game loop phases in stacking order (graph bands, summary bar and legend all use this order).
static const int kPhaseCount                 = 4;
static const ImU32 kPhaseColors[kPhaseCount] = {kColFixed, kColUpdate, kColRender, kColOther};
static const char* kPhaseNames[kPhaseCount]  = {"Fixed Update", "Update", "Render", "Other"};

// Where the frame graph is on screen and how samples map onto it (see DrawFrameGraph).
struct FrameGraphArea
{
	ImVec2 topLeft;
	ImVec2 bottomRight;
	float height;
	float displayMaxMs;  // the frame time at the top edge
	float sampleStep;    // px between samples
	int sampleCount;     // the newest sample sits on the right edge
};

static float GraphY(const FrameGraphArea& area, float ms)
{
	float t = std::clamp(ms / area.displayMaxMs, 0.0f, 1.0f);
	return area.bottomRight.y - t * area.height;
}

static float GraphX(const FrameGraphArea& area, int sampleIndex)
{
	return area.bottomRight.x - static_cast<float>(area.sampleCount - 1 - sampleIndex) * area.sampleStep;
}

static ImVec4 ToVec4(ImU32 color)
{
	return ImGui::ColorConvertU32ToFloat4(color);
}

static ImU32 WithAlpha(ImU32 color, unsigned alpha)
{
	return (color & 0x00FFFFFFu) | (alpha << 24);
}

// Sort orders for the profile tree (std::sort comparators).
static bool LongerDurationFirst(const Struktur::Debug::ProfileNode* lhs, const Struktur::Debug::ProfileNode* rhs)
{
	return lhs->duration_us > rhs->duration_us;
}

static bool LongerSelfDurationFirst(const Struktur::Debug::ProfileNode* lhs, const Struktur::Debug::ProfileNode* rhs)
{
	return lhs->self_duration_us > rhs->self_duration_us;
}

static bool MoreCallsFirst(const Struktur::Debug::ProfileNode* lhs, const Struktur::Debug::ProfileNode* rhs)
{
	return lhs->call_count > rhs->call_count;
}

static bool AlphabeticalByName(const Struktur::Debug::ProfileNode* lhs, const Struktur::Debug::ProfileNode* rhs)
{
	return lhs->name < rhs->name;
}

void Struktur::Debug::ProfilerWindow::Render(GameContext& context)
{
	if (!ImGui::Begin(m_name.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	auto& profiler = context.GetProfiler();
	if (!profiler.IsPaused())
	{
		SamplePhases(context);
	}

	DrawControls(context);
	ImGui::Separator();

	DrawFrameTimeGraph(context);
	ImGui::Spacing();
	DrawPhaseSummary(context);
	ImGui::Separator();

	float leftWidth = ImGui::GetContentRegionAvail().x * 0.5f;

	ImGui::BeginChild("TreeView", ImVec2(leftWidth, 0), true);
	const ProfileNode* rootNode = profiler.GetRootNode();
	if (rootNode && !rootNode->children.empty())
	{
		// Skip the synthetic "Frame" root itself (its total is not closed yet at this point in the frame) and
		// start the tree at its children - "Game Loop" and anything else top level.
		for (const auto* child : rootNode->children)
		{
			DrawProfileTree(context, child);
		}
	}
	else if (!profiler.IsPaused())
	{
		ImGui::TextDisabled("No profiling data yet...");
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Details", ImVec2(0, 0), true);
	if (m_selectedNode)
	{
		DrawNodeDetails(context, m_selectedNode);
		ImGui::Separator();
		const ProfileNode* chartNode = m_selectedNode->children.empty() ? profiler.GetRootNode() : m_selectedNode;
		DrawTimingBarChart(context, chartNode);
	}
	else
	{
		ImGui::TextDisabled("Select a node in the tree to view details");
	}
	ImGui::EndChild();

	ImGui::End();
}

void Struktur::Debug::ProfilerWindow::DrawControls(GameContext& context)
{
	auto& profiler = context.GetProfiler();
	bool isPaused  = profiler.IsPaused();

	if (ImGui::Button(isPaused ? "Resume" : "Pause"))
	{
		profiler.SetPaused(!isPaused);
	}
	ImGui::SameLine();
	if (isPaused)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[PAUSED]");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[LIVE]");
	}

	// Tree ordering - "Call order" leaves children exactly as the frame entered them (the default), the rest
	// re-sort for hunting down a specific hotspot.
	ImGui::TextUnformatted("Tree:");
	ImGui::SameLine();
	struct SortOption
	{
		const char* label;
		SortMode mode;
	};
	static const SortOption kSortOptions[] = {{"Call order", SortMode::CallOrder},
	                                          {"Total", SortMode::Duration},
	                                          {"Self", SortMode::SelfDuration},
	                                          {"Calls", SortMode::CallCount},
	                                          {"Name", SortMode::Name}};
	for (const SortOption& option : kSortOptions)
	{
		if (ImGui::RadioButton(option.label, m_sortMode == option.mode))
		{
			m_sortMode = option.mode;
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();

	ImGui::TextUnformatted("Graph Y:");
	ImGui::SameLine();
	if (ImGui::RadioButton("Auto", m_graphScaleMode == GraphScaleMode::Auto))
	{
		m_graphScaleMode = GraphScaleMode::Auto;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Fixed", m_graphScaleMode == GraphScaleMode::Fixed))
	{
		m_graphScaleMode = GraphScaleMode::Fixed;
	}
	if (m_graphScaleMode == GraphScaleMode::Fixed)
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90);
		ImGui::DragFloat("max ms", &m_graphFixedMaxMs, 0.5f, 2.0f, 200.0f, "%.1f");
	}
	ImGui::SameLine();
	ImGui::Checkbox("FPS guides", &m_showFpsGuides);
	ImGui::SameLine();
	ImGui::Checkbox("Phase bands", &m_showPhaseBands);

	if (ImGui::CollapsingHeader("Frame Rate Override & Lag  (editor only)"))
	{
		DrawOverrideControls(context);
	}
}

void Struktur::Debug::ProfilerWindow::DrawOverrideControls(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();

	ImGui::TextWrapped("Overrides the game's real VSync / target-FPS settings so you can preview how it plays at "
	                   "another framerate or under load. Nothing here is saved.");
	ImGui::Spacing();

	if (ImGui::Checkbox("Enable override", &gameData.frameRateOverrideEnabled))
	{
		// VSync would clamp the loop to the display refresh and defeat the override - suspend it while the
		// override is on, restore the real choice when it goes off. Not written to EngineSettings.
		context.GetGraphicsDevice().SetVSync(gameData.frameRateOverrideEnabled ? false : gameData.vsyncEnabled);
	}

	ImGui::BeginDisabled(!gameData.frameRateOverrideEnabled);

	bool capFps = gameData.overrideFps > 0;
	if (ImGui::Checkbox("Cap FPS", &capFps))
	{
		gameData.overrideFps = capFps ? 60 : 0;
	}
	ImGui::BeginDisabled(!capFps);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	int fps = gameData.overrideFps > 0 ? gameData.overrideFps : 60;
	if (ImGui::DragInt("Override FPS", &fps, 1.0f, 1, 1000))
	{
		gameData.overrideFps = fps;
	}
	ImGui::SameLine();
	// Quick presets.
	static const int kFpsPresets[] = {15, 30, 45, 60};
	for (int preset : kFpsPresets)
	{
		if (ImGui::SmallButton(std::to_string(preset).c_str()))
		{
			gameData.overrideFps = preset;
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
	ImGui::EndDisabled();

	ImGui::SetNextItemWidth(240);
	ImGui::SliderFloat("Artificial lag (ms / frame)", &gameData.artificialLagMs, 0.0f, 100.0f, "%.1f");

	ImGui::EndDisabled();

	ImGui::Spacing();
	if (ImGui::Button("Inject 250 ms spike"))
	{
		gameData.oneOffHitchMs = 250.0f;
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(one-off hitch - works even with override off)");
}

void Struktur::Debug::ProfilerWindow::DrawFrameTimeGraph(GameContext& context)
{
	if (m_phaseHistory.size() < 2)
	{
		ImGui::TextDisabled("Collecting frame data...");
		return;
	}

	float maxTime = 0.0f, minTime = 1e9f, avgTime = 0.0f;
	for (const auto& sample : m_phaseHistory)
	{
		maxTime = std::max(maxTime, sample.totalMs);
		minTime = std::min(minTime, sample.totalMs);
		avgTime += sample.totalMs;
	}
	avgTime /= static_cast<float>(m_phaseHistory.size());
	float lastMs = m_phaseHistory.back().totalMs;

	// Two distinct numbers: how much CPU work the game loop does (the graph), versus the real end-to-end frame
	// period from TimeSystem - the latter includes VSync wait and the frame cap, and is what the override moves.
	float frameMs = context.GetTimeSystem().unscaledDelta * 1000.0f;
	ImGui::Text("Game Loop: %.2f ms  (avg %.2f  min %.2f  max %.2f)", lastMs, avgTime, minTime, maxTime);
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.75f, 1.0f), "|  Frame: %.2f ms  (%.0f FPS)", frameMs,
	                   frameMs > 0.0f ? 1000.0f / frameMs : 0.0f);

	float displayMax;
	if (m_graphScaleMode == GraphScaleMode::Fixed)
	{
		displayMax = std::max(m_graphFixedMaxMs, 1.0f);
	}
	else
	{
		// Scale to the actual data (recent worst frame + headroom), NOT to the 60 FPS target. On a 144 Hz
		// monitor the trace would otherwise sit squashed against the bottom under a target line stuck near the
		// middle - this keeps the real frame times filling the graph. The target / FPS guides still draw when
		// they land in range, and get a small "off scale" marker at the top edge when they do not.
		displayMax = std::max({maxTime * 1.3f, avgTime * 2.0f, 2.0f});
	}
	displayMax = std::max(displayMax, 2.0f);

	float width = ImGui::GetContentRegionAvail().x;
	if (width < 32.0f)
	{
		return;
	}
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 p0      = ImGui::GetCursorScreenPos();
	float height   = 150.0f;
	ImVec2 p1      = ImVec2(p0.x + width, p0.y + height);

	ImGui::InvisibleButton("##frameGraph", ImVec2(width, height));

	dl->AddRectFilled(p0, p1, kColGraphBg);
	dl->PushClipRect(p0, p1, true);

	const int sampleCount = static_cast<int>(m_phaseHistory.size());
	const int cap         = static_cast<int>(m_maxPhaseHistory);
	const float step      = width / static_cast<float>(std::max(cap - 1, 1));
	const FrameGraphArea area{p0, p1, height, displayMax, step, sampleCount};

	if (m_showPhaseBands)
	{
		// Each phase is a band stacked on the ones before it, joined sample to sample.
		for (int i = 1; i < sampleCount; ++i)
		{
			const PhaseSample& previous             = m_phaseHistory[i - 1];
			const PhaseSample& current              = m_phaseHistory[i];
			const float previousPhases[kPhaseCount] = {previous.fixedMs, previous.updateMs, previous.renderMs,
			                                           previous.otherMs};
			const float currentPhases[kPhaseCount]  = {current.fixedMs, current.updateMs, current.renderMs,
			                                           current.otherMs};
			float x0                                = GraphX(area, i - 1);
			float x1                                = GraphX(area, i);
			float previousTop                       = 0.0f;
			float currentTop                        = 0.0f;
			for (int phase = 0; phase < kPhaseCount; ++phase)
			{
				float previousNext = previousTop + previousPhases[phase];
				float currentNext  = currentTop + currentPhases[phase];
				dl->AddQuadFilled(ImVec2(x0, GraphY(area, previousTop)), ImVec2(x1, GraphY(area, currentTop)),
				                  ImVec2(x1, GraphY(area, currentNext)), ImVec2(x0, GraphY(area, previousNext)),
				                  WithAlpha(kPhaseColors[phase], 115));
				previousTop = previousNext;
				currentTop  = currentNext;
			}
		}
	}

	for (int i = 1; i < sampleCount; ++i)
	{
		dl->AddLine(ImVec2(GraphX(area, i - 1), GraphY(area, m_phaseHistory[i - 1].totalMs)),
		            ImVec2(GraphX(area, i), GraphY(area, m_phaseHistory[i].totalMs)), kColFrameLine, 1.5f);
	}

	if (m_showFpsGuides)
	{
		// Reference lines at common refresh rates - a frame trace touching the "60 FPS" line means 16.7 ms
		// frames, and so on. The line closest to the target refresh doubles as the "budget" marker.
		const int fpsGuides[] = {30, 60, 120, 144};
		for (int fps : fpsGuides)
		{
			float ms = 1000.0f / static_cast<float>(fps);
			if (ms > displayMax)
			{
				continue;
			}
			float y = GraphY(area, ms);
			dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), kColGuide, 1.0f);
			char lbl[16];
			std::snprintf(lbl, sizeof(lbl), "%d FPS", fps);
			dl->AddText(ImVec2(p0.x + 4, y - 14), kColGuideText, lbl);
		}
	}

	dl->PopClipRect();

	if (ImGui::IsItemHovered())
	{
		float mx  = ImGui::GetIO().MousePos.x;
		int idx   = sampleCount - 1 - static_cast<int>(std::round((p1.x - mx) / step));
		if (idx >= 0 && idx < sampleCount)
		{
			const PhaseSample& sample = m_phaseHistory[idx];
			ImGui::BeginTooltip();
			ImGui::Text("Game Loop  %.2f ms", sample.totalMs);
			ImGui::TextColored(ToVec4(kColFixed), "  Fixed Update  %.2f ms", sample.fixedMs);
			ImGui::TextColored(ToVec4(kColUpdate), "  Update        %.2f ms", sample.updateMs);
			ImGui::TextColored(ToVec4(kColRender), "  Render        %.2f ms", sample.renderMs);
			ImGui::TextColored(ToVec4(kColOther), "  Other         %.2f ms", sample.otherMs);
			ImGui::EndTooltip();
		}
	}
}

void Struktur::Debug::ProfilerWindow::DrawPhaseSummary(GameContext& context)
{
	if (m_phaseHistory.empty())
	{
		return;
	}
	const PhaseSample& latest = m_phaseHistory.back();
	float total               = std::max(latest.totalMs, 0.0001f);

	float width = ImGui::GetContentRegionAvail().x;
	if (width < 32.0f)
	{
		return;
	}
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 p0      = ImGui::GetCursorScreenPos();
	float height   = 22.0f;
	ImGui::InvisibleButton("##phaseBar", ImVec2(width, height));

	const float phases[kPhaseCount] = {latest.fixedMs, latest.updateMs, latest.renderMs, latest.otherMs};

	// One segment per phase, sized by its share of the frame.
	float x = p0.x;
	for (int phase = 0; phase < kPhaseCount; ++phase)
	{
		float segmentWidth = width * (phases[phase] / total);
		dl->AddRectFilled(ImVec2(x, p0.y), ImVec2(x + segmentWidth, p0.y + height), kPhaseColors[phase]);
		x += segmentWidth;
	}
	dl->AddRect(p0, ImVec2(p0.x + width, p0.y + height), IM_COL32(0, 0, 0, 120));

	for (int phase = 0; phase < kPhaseCount; ++phase)
	{
		ImGui::ColorButton(kPhaseNames[phase], ToVec4(kPhaseColors[phase]),
		                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(12, 12));
		ImGui::SameLine();
		ImGui::Text("%s %.2f ms (%.0f%%)", kPhaseNames[phase], phases[phase], 100.0f * phases[phase] / total);
		ImGui::SameLine(0.0f, 16.0f);
	}
	ImGui::NewLine();
}

const Struktur::Debug::ProfileNode* Struktur::Debug::ProfilerWindow::FindNode(const ProfileNode* root, const char* name)
{
	if (!root)
	{
		return nullptr;
	}
	if (root->name == name)
	{
		return root;
	}
	for (const auto* child : root->children)
	{
		if (const ProfileNode* found = FindNode(child, name))
		{
			return found;
		}
	}
	return nullptr;
}

void Struktur::Debug::ProfilerWindow::SamplePhases(GameContext& context)
{
	auto& profiler          = context.GetProfiler();
	const ProfileNode* root = profiler.GetRootNode();
	if (!root)
	{
		return;
	}

	// The window renders from inside editor.Update(), before PROFILE_END_FRAME - so the root "Frame" total for
	// this frame is not known yet, but the "Game Loop" subtree has already closed. Break down the loop instead:
	// its own time minus the three phases is "Other" (queue flush, BeginFrame, gaps).
	const ProfileNode* loop  = FindNode(root, "Game Loop");
	const ProfileNode* fixed = FindNode(root, "Fixed Update");
	const ProfileNode* upd   = FindNode(root, "Update");
	const ProfileNode* ren   = FindNode(root, "Render");

	PhaseSample sample;
	sample.totalMs  = loop ? loop->duration_us / 1000.0f : 0.0f;
	sample.fixedMs  = fixed ? fixed->duration_us / 1000.0f : 0.0f;
	sample.updateMs = upd ? upd->duration_us / 1000.0f : 0.0f;
	sample.renderMs = ren ? ren->duration_us / 1000.0f : 0.0f;
	sample.otherMs  = std::max(0.0f, sample.totalMs - sample.fixedMs - sample.updateMs - sample.renderMs);

	m_phaseHistory.push_back(sample);
	while (m_phaseHistory.size() > m_maxPhaseHistory)
	{
		m_phaseHistory.pop_front();
	}
}

void Struktur::Debug::ProfilerWindow::DrawTimingBarChart(GameContext& context, const ProfileNode* rootNode)
{
	if (!rootNode || rootNode->duration_us == 0 || rootNode->children.empty())
	{
		return;
	}

	ImGui::Text("Children of \"%s\"", rootNode->name.c_str());
	ImGui::Separator();

	std::vector<const ProfileNode*> nodes(rootNode->children.begin(), rootNode->children.end());
	std::sort(nodes.begin(), nodes.end(), LongerDurationFirst);

	float targetMs = context.GetProfiler().GetTargetFrameTimeMs();
	for (const auto* node : nodes)
	{
		float nodeMs   = node->duration_us / 1000.0f;
		float fraction = static_cast<float>(node->duration_us) / static_cast<float>(rootNode->duration_us);

		ImVec4 color;
		if (nodeMs > targetMs * 0.5f)
		{
			color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
		}
		else if (nodeMs > targetMs * 0.25f)
		{
			color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
		}
		else
		{
			color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
		}

		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
		char label[256];
		std::snprintf(label, sizeof(label), "%s - %.2f ms (%.1f%%)", node->name.c_str(), nodeMs, fraction * 100.0f);
		ImGui::ProgressBar(fraction, ImVec2(-1, 0), label);
		ImGui::PopStyleColor();
	}

	ImGui::Text("Total: %.2f ms", rootNode->duration_us / 1000.0f);
}

void Struktur::Debug::ProfilerWindow::DrawProfileTree(GameContext& context, const ProfileNode* node)
{
	float durationMs     = node->duration_us / 1000.0f;
	float selfDurationMs = node->self_duration_us / 1000.0f;
	float targetMs       = context.GetProfiler().GetTargetFrameTimeMs();

	// Phase nodes get the shared palette colour and open by default; everything else is coloured by how hot it
	// is relative to the frame budget.
	bool isPhase   = false;
	ImVec4 color   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	if (node->name == "Fixed Update")
	{
		color   = ToVec4(kColFixed);
		isPhase = true;
	}
	else if (node->name == "Update" || node->name == "Update Processing")
	{
		color   = ToVec4(kColUpdate);
		isPhase = true;
	}
	else if (node->name == "Render")
	{
		color   = ToVec4(kColRender);
		isPhase = true;
	}
	else if (node->name == "Game Loop" || node->name == "Queue Flush")
	{
		color   = ToVec4(kColOther);
		isPhase = true;
	}
	else if (durationMs > targetMs)
	{
		color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
	}
	else if (durationMs > targetMs * 0.5f)
	{
		color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
	}
	else
	{
		color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
	}

	char label[256];
	std::snprintf(label, sizeof(label), "%s  -  %.2f ms  (self %.2f)  x%zu", node->name.c_str(), durationMs,
	              selfDurationMs, node->call_count);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (node == m_selectedNode)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (node->children.empty())
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (isPhase)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	}

	ImGui::PushStyleColor(ImGuiCol_Text, color);
	bool nodeOpen = ImGui::TreeNodeEx(node, flags, "%s", label);
	ImGui::PopStyleColor();

	if (ImGui::IsItemClicked())
	{
		m_selectedNode = node;
	}

	if (nodeOpen && !node->children.empty())
	{
		std::vector<const ProfileNode*> children(node->children.begin(), node->children.end());
		switch (m_sortMode)
		{
			case SortMode::CallOrder:
				// children are already stored in first-entered order - leave them
				break;
			case SortMode::Duration:
				std::sort(children.begin(), children.end(), LongerDurationFirst);
				break;
			case SortMode::SelfDuration:
				std::sort(children.begin(), children.end(), LongerSelfDurationFirst);
				break;
			case SortMode::CallCount:
				std::sort(children.begin(), children.end(), MoreCallsFirst);
				break;
			case SortMode::Name:
				std::sort(children.begin(), children.end(), AlphabeticalByName);
				break;
		}

		for (const auto* child : children)
		{
			DrawProfileTree(context, child);
		}

		ImGui::TreePop();
	}
}

void Struktur::Debug::ProfilerWindow::DrawNodeDetails(GameContext& context, const ProfileNode* node)
{
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", node->name.c_str());
	ImGui::Separator();

	float durationMs     = node->duration_us / 1000.0f;
	float selfDurationMs = node->self_duration_us / 1000.0f;
	float avgDurationMs  = durationMs / std::max(node->call_count, size_t(1));

	ImGui::Text("Total Time:    %.3f ms", durationMs);
	ImGui::Text("Self Time:     %.3f ms", selfDurationMs);
	ImGui::Text("Call Count:    %zu", node->call_count);
	ImGui::Text("Avg Time/Call: %.3f ms", avgDurationMs);
	ImGui::Text("Depth:         %zu", node->depth);

	if (node->parent && node->parent->duration_us > 0)
	{
		float percentOfParent = static_cast<float>(node->duration_us) / node->parent->duration_us * 100.0f;
		ImGui::Text("Of Parent:     %.1f%%", percentOfParent);
	}

	const ProfileNode* root = node;
	while (root->parent)
	{
		root = root->parent;
	}
	if (root->duration_us > 0 && node->depth > 0)
	{
		float percentOfFrame = static_cast<float>(node->duration_us) / root->duration_us * 100.0f;
		ImGui::Text("Of Frame:      %.1f%%", percentOfFrame);
	}

	if (!node->children.empty())
	{
		ImGui::Separator();
		ImGui::Text("Child Breakdown:");
		for (const auto* child : node->children)
		{
			float childMs  = child->duration_us / 1000.0f;
			float fraction = node->duration_us > 0
			                     ? static_cast<float>(child->duration_us) / static_cast<float>(node->duration_us)
			                     : 0.0f;
			char label[128];
			std::snprintf(label, sizeof(label), "%s (%.2f ms)", child->name.c_str(), childMs);
			ImGui::ProgressBar(fraction, ImVec2(-1, 0), label);
		}
	}
}
