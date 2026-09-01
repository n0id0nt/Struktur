#pragma once

#include <deque>

#include "Debug/Profiling/Profiler.h"
#include "EditorWindow.h"

namespace Struktur::Debug
{
class ProfilerWindow : public EditorWindow
{
public:
	ProfilerWindow()
	    : EditorWindow("Profiler")
	{
		m_selectedNode = nullptr;
		m_sortMode     = SortMode::CallOrder;
	}

	void Render(GameContext& context) override;

private:
	enum class SortMode
	{
		CallOrder,  // leave children in the order they were first entered this frame
		Duration,
		SelfDuration,
		CallCount,
		Name
	};

	enum class GraphScaleMode
	{
		Auto,   // track the recent worst frame (with headroom)
		Fixed,  // pinned ceiling the user picks
	};

	// Per-frame split of the top-level loop phases, kept in a small ring for the graph.
	struct PhaseSample
	{
		float fixedMs  = 0.0f;
		float updateMs = 0.0f;
		float renderMs = 0.0f;
		float otherMs  = 0.0f;
		float totalMs  = 0.0f;
	};

	void DrawControls(GameContext& context);
	void DrawOverrideControls(GameContext& context);
	void DrawFrameTimeGraph(GameContext& context);
	void DrawPhaseSummary(GameContext& context);
	void DrawProfileTree(GameContext& context, const ProfileNode* node);
	void DrawNodeDetails(GameContext& context, const ProfileNode* node);
	void DrawTimingBarChart(GameContext& context, const ProfileNode* node);

	void SamplePhases(GameContext& context);
	static const ProfileNode* FindNode(const ProfileNode* root, const char* name);

	const ProfileNode* m_selectedNode;
	SortMode m_sortMode;

	GraphScaleMode m_graphScaleMode = GraphScaleMode::Auto;
	float m_graphFixedMaxMs         = 33.3f;
	bool m_showFpsGuides            = true;
	bool m_showPhaseBands           = true;

	std::deque<PhaseSample> m_phaseHistory;
	size_t m_maxPhaseHistory = 240;
};
}  // namespace Struktur::Debug
