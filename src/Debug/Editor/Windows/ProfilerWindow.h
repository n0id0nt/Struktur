#pragma once

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
		m_sortMode     = SortMode::Duration;
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

	void DrawProfileTree(GameContext& context, const ProfileNode* node);
	void DrawNodeDetails(GameContext& context, const ProfileNode* node);
	void DrawFrameTimeGraph(GameContext& context);
	void DrawTimingBarChart(GameContext& context, const ProfileNode* node);
	void DrawControls(GameContext& context);
	void CollectAllNodes(GameContext& context, const ProfileNode* node, std::vector<const ProfileNode*>& nodes);

	const ProfileNode* m_selectedNode;
	SortMode m_sortMode;
};
}  // namespace Struktur::Debug
