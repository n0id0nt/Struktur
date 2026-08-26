#pragma once

namespace Struktur::Core
{
// Persisted frame-pacing preferences (VSync/FPS cap) - not EDITOR/DEBUG-gated since a shipped release build
// reads the same file the editor's Settings window writes, so the choice survives a relaunch either way. See
// GraphicsDevice::SetVSync (applies vsync) and GameData::targetFps (read by the frame-cap sleep in Game.cpp's
// desktop loop).
struct EngineSettings
{
	bool vsync    = true;
	int targetFps = 0;  // 0 = uncapped

	// Reads from the save-directory mount (same one FlagManager-driven saves use, see Game.cpp's
	// FileSystem::SetWriteDir/Mount call in InitialiseGame) - defaults are returned unchanged if no file exists
	// yet (first run) or it fails to parse.
	static EngineSettings Load();

	// Writes to the same location. Called once at boot is not enough on its own - callers should also call this
	// immediately whenever the user changes a setting (see SettingsWindow) so the choice isn't lost if the
	// process doesn't exit cleanly.
	static void Save(const EngineSettings& settings);
};
}  // namespace Struktur::Core
