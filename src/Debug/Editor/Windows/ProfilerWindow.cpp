#include "ProfilerWindow.h"
#include <imgui.h>
#include <algorithm>

void Struktur::Debug::ProfilerWindow::Render(GameContext& context)
{
    if (!ImGui::Begin(m_name.c_str(), &m_isOpen))
    {
        ImGui::End();
        return;
    }

    DrawControls();
    ImGui::Separator();

    // Frame time graph at top
    DrawFrameTimeGraph();
    ImGui::Separator();

    // Main content split into tree and details
    float leftWidth = ImGui::GetContentRegionAvail().x * 0.5f;

    ImGui::BeginChild("TreeView", ImVec2(leftWidth, 0), true);

    auto& profiler = Profiler::Get();
    const ProfileNode* rootNode = profiler.GetRootNode();

    if (rootNode && !rootNode->children.empty())
    {
        DrawProfileTree(rootNode);
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
        DrawNodeDetails(m_selectedNode);
        ImGui::Separator();
        DrawTimingBarChart(profiler.GetRootNode());
    }
    else
    {
        ImGui::TextDisabled("Select a node to view details");
    }
    ImGui::EndChild();

    ImGui::End();
}

void Struktur::Debug::ProfilerWindow::DrawControls()
{
    auto& profiler = Profiler::Get();
    bool isPaused = profiler.IsPaused();

    if (ImGui::Button(isPaused ? "Resume" : "Pause"))
    {
        profiler.SetPaused(!isPaused);
    }

    ImGui::SameLine();
    ImGui::Text("Sort by:");
    ImGui::SameLine();

    if (ImGui::RadioButton("Duration", m_sortMode == SortMode::Duration))
        m_sortMode = SortMode::Duration;
    ImGui::SameLine();
    if (ImGui::RadioButton("Self", m_sortMode == SortMode::SelfDuration))
        m_sortMode = SortMode::SelfDuration;
    ImGui::SameLine();
    if (ImGui::RadioButton("Calls", m_sortMode == SortMode::CallCount))
        m_sortMode = SortMode::CallCount;

    ImGui::SameLine();
    if (isPaused)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[PAUSED]");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[LIVE]");
    }

    // Target frame time setting
    ImGui::SameLine();
    float targetMs = profiler.GetTargetFrameTimeMs();
    ImGui::SetNextItemWidth(80);
    if (ImGui::DragFloat("Target (ms)", &targetMs, 0.1f, 1.0f, 100.0f, "%.2f"))
    {
        profiler.SetTargetFrameTimeMs(targetMs);
    }
}

void Struktur::Debug::ProfilerWindow::DrawFrameTimeGraph()
{
    auto& profiler = Profiler::Get();
    const auto& history = profiler.GetFrameTimeHistory();

    if (history.empty()) return;

    // Calculate stats
    float maxTime = 0.0f;
    float avgTime = 0.0f;
    float minTime = 999999.0f;

    for (float time : history)
    {
        maxTime = std::max(maxTime, time);
        minTime = std::min(minTime, time);
        avgTime += time;
    }
    avgTime /= history.size();

    float targetMs = profiler.GetTargetFrameTimeMs();
    float graphHeight = 100.0f;

    // Draw stats
    ImGui::Text("Frame Time - Avg: %.2fms | Min: %.2fms | Max: %.2fms | FPS: %.1f",
        avgTime, minTime, maxTime, 1000.0f / avgTime);

    // Draw graph
    std::vector<float> values(history.begin(), history.end());

    char overlay[64];
    snprintf(overlay, sizeof(overlay), "%.2f ms", history.back());

    ImGui::PlotLines("##frametime", values.data(), static_cast<int>(values.size()),
        0, overlay, 0.0f, maxTime * 1.2f,
        ImVec2(ImGui::GetContentRegionAvail().x, graphHeight));

    // Draw target line overlay
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 graphMin = ImGui::GetItemRectMin();
    ImVec2 graphMax = ImGui::GetItemRectMax();

    float targetY = graphMax.y - (targetMs / (maxTime * 1.2f)) * graphHeight;
    drawList->AddLine(
        ImVec2(graphMin.x, targetY),
        ImVec2(graphMax.x, targetY),
        IM_COL32(255, 255, 0, 128), 2.0f);
}

void Struktur::Debug::ProfilerWindow::DrawTimingBarChart(const ProfileNode* rootNode)
{
    if (!rootNode || rootNode->duration_us == 0) return;

    ImGui::Text("Timing Bar Chart");
    ImGui::Separator();

    // Collect all nodes at depth 1 (immediate children of root)
    std::vector<const ProfileNode*> topLevelNodes;
    for (auto child : rootNode->children)
    {
        topLevelNodes.push_back(child);
    }

    // Sort by duration
    std::sort(topLevelNodes.begin(), topLevelNodes.end(),
        [](const ProfileNode* a, const ProfileNode* b) {
            return a->duration_us > b->duration_us;
        });

    float totalMs = rootNode->duration_us / 1000.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;

    // Draw bars
    for (auto node : topLevelNodes)
    {
        float nodeMs = node->duration_us / 1000.0f;
        float fraction = (float)node->duration_us / rootNode->duration_us;
        float barWidth = availableWidth * fraction;

        // Color based on timing
        ImVec4 color;
        if (nodeMs > 8.0f) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
        else if (nodeMs > 4.0f) color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);  // Yellow
        else color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);  // Green

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);

        char label[256];
        snprintf(label, sizeof(label), "%.2fms (%.1f%%)", nodeMs, fraction * 100.0f);

        ImGui::ProgressBar(fraction, ImVec2(barWidth, 0), "");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(color, "%s - %s", node->name.c_str(), label);

        ImGui::PopStyleColor();
    }

    ImGui::Text("Total Frame Time: %.2fms", totalMs);
}

void Struktur::Debug::ProfilerWindow::DrawProfileTree(const ProfileNode* node)
{
    float durationMs = node->duration_us / 1000.0f;
    float selfDurationMs = node->self_duration_us / 1000.0f;

    // Color code
    ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    float targetMs = Profiler::Get().GetTargetFrameTimeMs();

    if (durationMs > targetMs) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    else if (durationMs > targetMs * 0.5f) color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
    else color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, color);

    char label[256];
    snprintf(label, sizeof(label), "%s (%.2fms / %.2fms self) [%zu calls]",
        node->name.c_str(), durationMs, selfDurationMs, node->call_count);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (node == m_selectedNode)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (node->children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool nodeOpen = ImGui::TreeNodeEx(node, flags, "%s", label);

    ImGui::PopStyleColor();

    if (ImGui::IsItemClicked())
    {
        m_selectedNode = node;
    }

    if (nodeOpen && !node->children.empty())
    {
        // Sort children
        std::vector<ProfileNode*> sortedChildren = node->children;

        switch (m_sortMode)
        {
        case SortMode::Duration:
            std::sort(sortedChildren.begin(), sortedChildren.end(),
                [](const ProfileNode* a, const ProfileNode* b) {
                    return a->duration_us > b->duration_us;
                });
            break;
        case SortMode::SelfDuration:
            std::sort(sortedChildren.begin(), sortedChildren.end(),
                [](const ProfileNode* a, const ProfileNode* b) {
                    return a->self_duration_us > b->self_duration_us;
                });
            break;
        case SortMode::CallCount:
            std::sort(sortedChildren.begin(), sortedChildren.end(),
                [](const ProfileNode* a, const ProfileNode* b) {
                    return a->call_count > b->call_count;
                });
            break;
        case SortMode::Name:
            std::sort(sortedChildren.begin(), sortedChildren.end(),
                [](const ProfileNode* a, const ProfileNode* b) {
                    return a->name < b->name;
                });
            break;
        }

        for (auto child : sortedChildren)
        {
            DrawProfileTree(child);
        }

        ImGui::TreePop();
    }
}

void Struktur::Debug::ProfilerWindow::DrawNodeDetails(const ProfileNode* node)
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", node->name.c_str());
    ImGui::Separator();

    float durationMs = node->duration_us / 1000.0f;
    float selfDurationMs = node->self_duration_us / 1000.0f;
    float avgDurationMs = durationMs / std::max(node->call_count, size_t(1));

    ImGui::Text("Total Time: %.3f ms", durationMs);
    ImGui::Text("Self Time: %.3f ms", selfDurationMs);
    ImGui::Text("Call Count: %zu", node->call_count);
    ImGui::Text("Avg Time/Call: %.3f ms", avgDurationMs);
    ImGui::Text("Depth: %zu", node->depth);

    // Percentages
    if (node->parent && node->parent->duration_us > 0)
    {
        float percentOfParent = (float)node->duration_us / node->parent->duration_us * 100.0f;
        ImGui::Text("Of Parent: %.1f%%", percentOfParent);
    }

    const ProfileNode* root = node;
    while (root->parent) root = root->parent;
    if (root->duration_us > 0 && node->depth > 0)
    {
        float percentOfFrame = (float)node->duration_us / root->duration_us * 100.0f;
        ImGui::Text("Of Frame: %.1f%%", percentOfFrame);
    }

    // Child breakdown
    if (!node->children.empty())
    {
        ImGui::Separator();
        ImGui::Text("Child Breakdown:");

        for (auto child : node->children)
        {
            float childMs = child->duration_us / 1000.0f;
            float fraction = (float)child->duration_us / node->duration_us;

            char label[128];
            snprintf(label, sizeof(label), "%.2fms", childMs);
            ImGui::ProgressBar(fraction, ImVec2(-1, 0), child->name.c_str());
            ImGui::SameLine();
            ImGui::Text("%s", label);
        }
    }
}