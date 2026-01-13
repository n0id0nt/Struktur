#pragma once

#include "EditorWindow.h"

#include "Debug/Profiling/Profiler.h"

namespace Struktur::Debug
{
    class ProfilerWindow : public EditorWindow
    {
    public:
        ProfilerWindow() : EditorWindow("Profiler")
        {
            m_selectedNode = nullptr;
            m_sortMode = SortMode::Duration;
        }

        void Render(GameContext& context) override;

    private:
        enum class SortMode
        {
            Duration,
            SelfDuration,
            CallCount,
            Name
        };

        void DrawProfileTree(const ProfileNode* node);
        void DrawNodeDetails(const ProfileNode* node);
        void DrawFrameTimeGraph();
        void DrawTimingBarChart(const ProfileNode* node);
        void DrawControls();
        void CollectAllNodes(const ProfileNode* node, std::vector<const ProfileNode*>& nodes);

        const ProfileNode* m_selectedNode;
        SortMode m_sortMode;
    };
}