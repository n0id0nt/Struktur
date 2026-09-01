#include "ProfilerWindow.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "Engine/Core/GameData.h"
#include "Engine/Core/TimeSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/GraphicsDevice.h"

namespace
{
// Shared phase palette - used by the graph bands, the stacked summary bar and the tree headers so the same
// phase always reads as the same colour everywhere in the window.
const ImU32 kColFixed     = IM_COL32(232, 151, 58, 255);
const ImU32 kColUpdate    = IM_COL32(80, 150, 232, 255);
const ImU32 kColRender    = IM_COL32(92, 200, 122, 255);
const ImU32 kColOther     = IM_COL32(150, 150, 160, 255);
const ImU32 kColFrameLine = IM_COL32(240, 240, 245, 255);
const ImU32 kColGuide     = IM_COL32(255, 255, 255, 38);
const ImU32 kColGuideText = IM_COL32(200, 200, 210, 150);
const ImU32 kColGraphBg   = IM_COL32(18, 18, 22, 255);

ImVec4 ToVec4(ImU32 c)
{
	return ImGui::ColorConvertU32ToFloat4(c);
}

ImU32 WithAlpha(ImU32 c, unsigned a)
{
	return (c & 0x00FFFFFFu) | (a << 24);
}
}  // namespace

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
	auto sortRadio = [&](const char* label, SortMode mode)
	{
		if (ImGui::RadioButton(label, m_sortMode == mode))
		{
			m_sortMode = mode;
		}
		ImGui::SameLine();
	};
	sortRadio("Call order", SortMode::CallOrder);
	sortRadio("Total", SortMode::Duration);
	sortRadio("Self", SortMode::SelfDuration);
	sortRadio("Calls", SortMode::CallCount);
	if (ImGui::RadioButton("Name", m_sortMode == SortMode::Name))
	{
		m_sortMode = SortMode::Name;
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
	auto fpsBtn = [&](const char* label, int v)
	{
		if (ImGui::SmallButton(label))
		{
			gameData.overrideFps = v;
		}
		ImGui::SameLine();
	};
	fpsBtn("15", 15);
	fpsBtn("30", 30);
	fpsBtn("45", 45);
	fpsBtn("60", 60);
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
	for (const auto& s : m_phaseHistory)
	{
		maxTime = std::max(maxTime, s.totalMs);
		minTime = std::min(minTime, s.totalMs);
		avgTime += s.totalMs;
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

	auto yFor = [&](float ms)
	{
		float t = std::clamp(ms / displayMax, 0.0f, 1.0f);
		return p1.y - t * height;
	};

	const int n     = static_cast<int>(m_phaseHistory.size());
	const int cap   = static_cast<int>(m_maxPhaseHistory);
	const float step = width / static_cast<float>(std::max(cap - 1, 1));
	auto xFor = [&](int i) { return p1.x - static_cast<float>(n - 1 - i) * step; };

	if (m_showPhaseBands)
	{
		for (int i = 1; i < n; ++i)
		{
			const PhaseSample& a = m_phaseHistory[i - 1];
			const PhaseSample& b = m_phaseHistory[i];
			float x0             = xFor(i - 1);
			float x1             = xFor(i);
			float accA = 0.0f, accB = 0.0f;

			auto band = [&](float av, float bv, ImU32 col)
			{
				float a1 = accA + av;
				float b1 = accB + bv;
				dl->AddQuadFilled(ImVec2(x0, yFor(accA)), ImVec2(x1, yFor(accB)), ImVec2(x1, yFor(b1)),
				                  ImVec2(x0, yFor(a1)), WithAlpha(col, 115));
				accA = a1;
				accB = b1;
			};
			band(a.fixedMs, b.fixedMs, kColFixed);
			band(a.updateMs, b.updateMs, kColUpdate);
			band(a.renderMs, b.renderMs, kColRender);
			band(a.otherMs, b.otherMs, kColOther);
		}
	}

	for (int i = 1; i < n; ++i)
	{
		dl->AddLine(ImVec2(xFor(i - 1), yFor(m_phaseHistory[i - 1].totalMs)),
		            ImVec2(xFor(i), yFor(m_phaseHistory[i].totalMs)), kColFrameLine, 1.5f);
	}

	if (m_showFpsGuides)
	{
		// Reference lines at common refresh rates - a frame trace touching the "60 FPS" line means 16.7 ms
		// frames, and so on. The line closest to the target refresh doubles as the "budget" marker.
		const int fpsGuides[] = {30, 60, 120, 144};
		for (int f : fpsGuides)
		{
			float ms = 1000.0f / static_cast<float>(f);
			if (ms > displayMax)
			{
				continue;
			}
			float y = yFor(ms);
			dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), kColGuide, 1.0f);
			char lbl[16];
			std::snprintf(lbl, sizeof(lbl), "%d FPS", f);
			dl->AddText(ImVec2(p0.x + 4, y - 14), kColGuideText, lbl);
		}
	}

	dl->PopClipRect();

	if (ImGui::IsItemHovered())
	{
		float mx  = ImGui::GetIO().MousePos.x;
		int idx   = n - 1 - static_cast<int>(std::round((p1.x - mx) / step));
		if (idx >= 0 && idx < n)
		{
			const PhaseSample& s = m_phaseHistory[idx];
			ImGui::BeginTooltip();
			ImGui::Text("Game Loop  %.2f ms", s.totalMs);
			ImGui::TextColored(ToVec4(kColFixed), "  Fixed Update  %.2f ms", s.fixedMs);
			ImGui::TextColored(ToVec4(kColUpdate), "  Update        %.2f ms", s.updateMs);
			ImGui::TextColored(ToVec4(kColRender), "  Render        %.2f ms", s.renderMs);
			ImGui::TextColored(ToVec4(kColOther), "  Other         %.2f ms", s.otherMs);
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
	const PhaseSample& s = m_phaseHistory.back();
	float total          = std::max(s.totalMs, 0.0001f);

	float width = ImGui::GetContentRegionAvail().x;
	if (width < 32.0f)
	{
		return;
	}
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 p0      = ImGui::GetCursorScreenPos();
	float height   = 22.0f;
	ImGui::InvisibleButton("##phaseBar", ImVec2(width, height));

	float x = p0.x;
	auto seg = [&](float ms, ImU32 col)
	{
		float w = width * (ms / total);
		dl->AddRectFilled(ImVec2(x, p0.y), ImVec2(x + w, p0.y + height), col);
		x += w;
	};
	seg(s.fixedMs, kColFixed);
	seg(s.updateMs, kColUpdate);
	seg(s.renderMs, kColRender);
	seg(s.otherMs, kColOther);
	dl->AddRect(p0, ImVec2(p0.x + width, p0.y + height), IM_COL32(0, 0, 0, 120));

	auto legend = [&](const char* name, float ms, ImU32 col)
	{
		ImGui::ColorButton(name, ToVec4(col), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
		                   ImVec2(12, 12));
		ImGui::SameLine();
		ImGui::Text("%s %.2f ms (%.0f%%)", name, ms, 100.0f * ms / total);
		ImGui::SameLine(0.0f, 16.0f);
	};
	legend("Fixed Update", s.fixedMs, kColFixed);
	legend("Update", s.updateMs, kColUpdate);
	legend("Render", s.renderMs, kColRender);
	legend("Other", s.otherMs, kColOther);
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

	PhaseSample s;
	s.totalMs  = loop ? loop->duration_us / 1000.0f : 0.0f;
	s.fixedMs  = fixed ? fixed->duration_us / 1000.0f : 0.0f;
	s.updateMs = upd ? upd->duration_us / 1000.0f : 0.0f;
	s.renderMs = ren ? ren->duration_us / 1000.0f : 0.0f;
	s.otherMs  = std::max(0.0f, s.totalMs - s.fixedMs - s.updateMs - s.renderMs);

	m_phaseHistory.push_back(s);
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
	std::sort(nodes.begin(), nodes.end(),
	          [](const ProfileNode* a, const ProfileNode* b) { return a->duration_us > b->duration_us; });

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
				std::sort(children.begin(), children.end(),
				          [](const ProfileNode* a, const ProfileNode* b) { return a->duration_us > b->duration_us; });
				break;
			case SortMode::SelfDuration:
				std::sort(children.begin(), children.end(), [](const ProfileNode* a, const ProfileNode* b)
				          { return a->self_duration_us > b->self_duration_us; });
				break;
			case SortMode::CallCount:
				std::sort(children.begin(), children.end(),
				          [](const ProfileNode* a, const ProfileNode* b) { return a->call_count > b->call_count; });
				break;
			case SortMode::Name:
				std::sort(children.begin(), children.end(),
				          [](const ProfileNode* a, const ProfileNode* b) { return a->name < b->name; });
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
