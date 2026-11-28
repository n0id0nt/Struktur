#pragma once

#include <string>

namespace Struktur
{
class GameContext;

namespace Wren
{
struct BindingRegistry;
}
}  // namespace Struktur

// The parts of a game's startup, frame and shutdown that every game shares. A game's Game.cpp calls them in its own
// order, around its own work. They need the ecs, wren and ui libraries.
namespace Struktur::App
{
// Text faded in and out before the game starts.
struct SplashScreen
{
	const char* text;
	const char* font;  // engine file system path
	int fontSize;      // the size the font is loaded at
	bool pixelFont;
};

// Adds a game's own render views (GraphicsDevice::AddView). They draw under the debug drawing and the UI.
using AddViewsFunction = void (*)(GameContext& context);
using FrameFunction    = void (*)(GameContext& context);

// Startup, in this order.

// Starts the engine file system and, outside the editor, mounts the game's assets.
void MountAssets();
#ifdef EDITOR
// Opens the editor's window, with the game's views, and mounts the game's asset folder (STRUKTUR_ASSETS_DIR).
void OpenEditor(GameContext& context, AddViewsFunction addGameViews);
#endif
// Starts the script engine, with the bindings given to WrenScriptEngine::SetBindings, and loads the game's scripts.
void LoadScripts(GameContext& context);
#ifndef EDITOR
// Opens the game's window at the size its scripts asked for, with the game's views.
void OpenWindow(GameContext& context, AddViewsFunction addGameViews);
#endif
// Sends the window's touch and dropped-file events to input and the event manager.
void ForwardWindowEvents(GameContext& context);
// Mounts the folder saves are written to, and returns it.
std::string MountSaves(GameContext& context);
// Applies the player's saved engine settings (vsync, frame rate cap).
void ApplyEngineSettings(GameContext& context);
// Starts the physics world, in builds that have one.
void StartPhysics(GameContext& context);

// One frame.

// Window events, input, time and finished jobs. Sets the game state to QUIT when the window is closing.
void BeginFrame(GameContext& context);
// A frame of the splash screen. Moves the game state to GAME when it is over.
void SplashScreenFrame(GameContext& context, const SplashScreen& splash);
// A frame of the game: fixed steps, update systems, queued object changes, render systems and the editor. In the
// editor the update only runs while playing.
void PlayFrame(GameContext& context);
// Leaves the loop RunLoop is running.
void StopLoop(GameContext& context);
// Starts the jobs that overlap the next frame.
void EndFrame(GameContext& context);
// Calls frame until the game state is QUIT, keeping to the frame rate cap. On the web the browser runs the loop.
void RunLoop(GameContext& context, FrameFunction frame);

// Restarting and shutting down.

// Forgets what the last run of the scripts left in flags, input and button hints.
void ResetPlayState(GameContext& context);
// Shuts the scripts down and removes what they made: states, UI and entities.
void ClearScriptWorld(GameContext& context);
// Clears the libraries' services behind the scripts, ending with the script engine. A game unloads what it holds
// outside the entities before or after this, as long as it's before the resources go.
void ClearScriptServices(GameContext& context);
// Checks nothing still holds a game resource, then frees them. For a restart.
void ReleaseGameResources(GameContext& context);
// The end of a game's exit: resources, the editor and the file system.
void CloseGame(GameContext& context);
// The last call: destroys the editor's ImGui context and every service.
void Shutdown(GameContext& context);
}  // namespace Struktur::App
