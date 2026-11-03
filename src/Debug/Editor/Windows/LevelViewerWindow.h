#pragma once

#include <string>
#include <vector>

#include "EditorWindow.h"

namespace Struktur::Debug
{
class LevelViewer;

// Picks an LDtk world to show in the Game Viewport through LevelViewer, and lists its levels.
class LevelViewerWindow : public EditorWindow
{
public:
	explicit LevelViewerWindow(LevelViewer* levelViewer);

	void Initialise(GameContext& context) override;
	void Render(GameContext& context) override;

private:
	void RefreshWorldFiles();
	void RenderLevelList(GameContext& context);

	LevelViewer* m_levelViewer;
	// Virtual paths of every .ldtk under LEVELS_DIRECTORY.
	std::vector<std::string> m_worldFiles;
	int m_selectedWorldFile = 0;
};
}  // namespace Struktur::Debug
