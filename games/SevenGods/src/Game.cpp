// Seven Gods: what it adds to the engine's startup, frame and shutdown (Engine/App). The game itself is in assets/.
#include "Game.h"

#include <iterator>
#include <string>

#include "Debug/Assertions.h"
#include "Engine/App/App.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/Camera2DSystem.h"
#include "Engine/ECS/System/EventSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/ParticleSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/SoundSystem.h"
#include "Engine/ECS/System/SpriteRenderSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/UIRenderSystem.h"
#include "Engine/ECS/System/UIsystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/System/WrenStateSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"
#include "SevenGodsContext.h"
#ifdef EDITOR
	#include "Debug/Editor/DebugSystem.h"
	#include "Debug/Editor/Editor.h"
#endif

namespace Struktur
{
// Encodes this game's saves (FileSystem::SetSaveKey). Keep it: changing it makes existing saves unreadable.
static constexpr uint8_t k_saveKey[] = {0x4B, 0x72, 0x69, 0x63, 0x6B, 0x65, 0x74, 0x47,
                                        0x61, 0x6D, 0x65, 0x53, 0x61, 0x76, 0x65, 0x31};

#ifdef EDITOR
static constexpr App::SplashScreen k_splashScreen{"STRUKTUR", "Fonts/machine-std/machine-std-regular.ttf", 120, false};
#else
// The game's pixel font (assets/Scripts/UI/Theme/UIKit.wren's font) at its 9px design size, sampled nearest-neighbour
// - drawn at 120 it snaps to the nearest whole multiple (see Text::GlyphScale).
static constexpr App::SplashScreen k_splashScreen{"Seven Gods", "UI/Font/NormalFont.ttf", 9, true};
#endif

static void RegisterSystems(System::SystemManager& systemManager)
{
	// need a way to have helper systems with out an empty update
	systemManager.AddHelperSystem<System::HierarchySystem>();
	systemManager.AddHelperSystem<System::TransformSystem>();
	systemManager.AddHelperSystem<System::ShaderSystem>();
	systemManager.AddUpdateSystem<System::WrenStateSystem>();
	systemManager.AddFixedUpdateSystem<System::WrenStateFixedUpdateSystem>();
	systemManager.AddFixedUpdateSystem<System::WrenScriptFixedUpdateSystem>();
	// Fixed-cadence Box2D step only - kept separate from PhysicsSystem's own per-frame Update (registered below,
	// right before the camera) which handles the Transform<->physics sync. See PhysicsFixedStepSystem.
	systemManager.AddFixedUpdateSystem<System::PhysicsFixedStepSystem>();
	systemManager.AddUpdateSystem<System::WrenScriptSystem>();
	systemManager.AddUpdateSystem<System::EventSystem>();
	systemManager.AddUpdateSystem<System::PhysicsSystem>();
	systemManager.AddUpdateSystem<System::Camera2DSystem>();
	systemManager.AddUpdateSystem<System::AnimationSystem>();
	systemManager.AddUpdateSystem<System::ParticleSystem>();
	systemManager.AddUpdateSystem<System::UISystem>();
	systemManager.AddUpdateSystem<System::SoundSystem>();
	// Render systems draw in this order.
	systemManager.AddRenderSystem<System::SpriteRenderSystem>();
#ifdef EDITOR
	systemManager.AddRenderSystem<System::DebugSystem>();
#endif
	systemManager.AddRenderSystem<System::UIRenderSystem>();
	// Worker-safe systems (SystemAccess) run side by side on the job system; this runs them all on the main thread.
	if (Core::CommandLine::HasFlag("--serial-systems"))
	{
		systemManager.SetParallel(false);
		DEBUG_INFO("--serial-systems - every system runs on the main thread");
	}
}

#ifdef EDITOR
// Back to nothing loaded.
static void ClearDebugGame(GameContext& context)
{
	App::ResetPlayState(context);
	App::ClearScriptWorld(context);
	App::ClearScriptServices(context);
	App::ReleaseGameResources(context);
}

// Nothing loaded, but ready to show a scene: what the level viewer opens a world into.
static void ResetDebugGame(GameContext& context)
{
	ClearDebugGame(context);
	App::LoadScripts(context);
	App::StartPhysics(context);
}

// The editor's Play and Refresh.
static void StartDebugGame(GameContext& context)
{
	ResetDebugGame(context);

	DEBUG_INFO("[Start] Wren State");
	context.GetWrenStateManager().Start(context);
}
#endif

static void InitialiseGame(GameContext& context)
{
	FileSystem::SetSaveKey({std::begin(k_saveKey), std::end(k_saveKey)});
	App::MountAssets();
#ifdef EDITOR
	// The 2D world draws in the engine's own game view, so there are no views to add.
	App::OpenEditor(context, nullptr);
	// ClearDebugGame is the editor's Stop.
	context.GetEditor().SetPlayControls({StartDebugGame, ClearDebugGame, ResetDebugGame});
#endif

	context.GetWrenScriptEngine().SetBindings(Wren::RegisterEngineBindings);
	App::LoadScripts(context);
#ifndef EDITOR
	App::OpenWindow(context, nullptr);
#endif
	App::ForwardWindowEvents(context);

	App::MountSaves(context);
	// Audio is set up by GameContext's constructor (SDL3_mixer), so it's already available here.
	App::ApplyEngineSettings(context);

	context.GetGameObjectManager().CreateObjectCallBack(context);
	App::StartPhysics(context);
	RegisterSystems(context.GetSystemManager());

	DEBUG_INFO("Game Data Loaded");

	// Release builds start the scripts at once. The editor also loads the first scene, so the Game Viewport shows it
	// before Play is pressed - nothing updates until Play or Refresh restarts it.
#if !defined(EDITOR)
	context.GetWrenStateManager().Start(context);
#endif
}

static void ExitGame(GameContext& context)
{
	App::ClearScriptWorld(context);
	App::ClearScriptServices(context);
	App::CloseGame(context);
}

static void Frame(GameContext& context)
{
	App::BeginFrame(context);
	switch (context.GetGameData().gameState)
	{
		case Core::GameState::SPLASH_SCREEN:
			App::SplashScreenFrame(context, k_splashScreen);
			break;

		case Core::GameState::GAME:
			App::PlayFrame(context);
			break;

		default:
			App::StopLoop(context);
			break;
	}
	App::EndFrame(context);
}

void Game()
{
	SevenGodsContext context;
	InitialiseGame(context);
	App::RunLoop(context, Frame);
	ExitGame(context);
	App::Shutdown(context);
}
}  // namespace Struktur
