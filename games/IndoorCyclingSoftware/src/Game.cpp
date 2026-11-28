// The indoor cycling game: what it adds to the engine's startup, frame and shutdown (Engine/App).
#include "Game.h"

#include <cstdio>
#include <iterator>
#include <string>

#include "Debug/Assertions.h"
#include "Engine/App/App.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Systems/CameraSystem.h"
#include "Engine/ECS/System/EventSystem.h"
#include "Engine/ECS/System/FitnessSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Systems/PacerSystem.h"
#include "Systems/RideSystem.h"
#include "Systems/RiderAnimationSystem.h"
#include "Engine/ECS/System/SoundSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/UIRenderSystem.h"
#include "Engine/ECS/System/UIsystem.h"
#include "Systems/WorldRenderSystem.h"
#include "Systems/WorldSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/System/WrenStateSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Fitness/FitnessManager.h"
#include "Renderer/WorldRenderer.h"
#include "RideContext.h"
#include "RideIntegration/RideService.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"
#include "World/World.h"
#include "World/WorldAssetLoader.h"
#include "World/WorldLifecycle.h"
#include "Bindings/RideBindings.h"
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

static void RegisterBindings(Wren::BindingRegistry& registry)
{
	Wren::RegisterEngineBindings(registry);
	Wren::RegisterRideBindings(registry);
	Wren::RegisterWorldBindings(registry);
}

// The road world draws in its own views, under the UI.
static void AddViews(GameContext& context)
{
	RideContext::From(context).GetWorldRenderer().AddViews(context.GetGraphicsDevice());
}

static void RegisterSystems(System::SystemManager& systemManager)
{
	// need a way to have helper systems with out an empty update
	systemManager.AddHelperSystem<System::HierarchySystem>();
	systemManager.AddHelperSystem<System::TransformSystem>();
	systemManager.AddUpdateSystem<System::WrenStateSystem>();
	systemManager.AddFixedUpdateSystem<System::WrenStateFixedUpdateSystem>();
	systemManager.AddFixedUpdateSystem<System::WrenScriptFixedUpdateSystem>();
	// Before WrenScriptSystem/EventSystem so fitness events queued this frame are delivered this frame.
	systemManager.AddUpdateSystem<System::FitnessSystem>();
	systemManager.AddUpdateSystem<System::RideSystem>();
	// The world: advance the road and the player, then move what rides on it, then the camera that follows.
	systemManager.AddUpdateSystem<System::WorldSystem>();
	systemManager.AddUpdateSystem<System::PacerSystem>();
	systemManager.AddUpdateSystem<System::RiderAnimationSystem>();
	systemManager.AddUpdateSystem<System::CameraSystem>();
	systemManager.AddUpdateSystem<System::WrenScriptSystem>();
	systemManager.AddUpdateSystem<System::EventSystem>();
	systemManager.AddUpdateSystem<System::UISystem>();
	systemManager.AddUpdateSystem<System::SoundSystem>();
	// Render systems draw in this order, each into its own views (see AddViews).
	systemManager.AddRenderSystem<System::WorldRenderSystem>();
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

// What the ride sets up once the saves are mounted.
static void StartRide(RideContext& context, const std::string& saveDir)
{
	// Remember the player's trainer / HR strap / power meter next to the saves. Loading only reads the choices;
	// nothing connects until the game calls FitnessManager::ConnectRememberedDevices.
	context.GetFitnessManager().SetRegistryPath(saveDir + "/fitness_devices.json");
	context.GetFitnessManager().LoadRegistry();
	context.GetRideService().Initialise(saveDir);
	context.GetWorld().SetAssets(World::LoadWorldAssets());
	if (Core::CommandLine::HasFlag("--list-missing-assets"))
	{
		// Prints every ride-world art role still using a placeholder (Markdown, same table as docs/missing-assets.md)
		// and quits before the first frame.
		const World::AssetManifest& manifest = context.GetWorld().GetAssets().manifest;
		const std::string table              = World::GetMissingAssetsMarkdown(manifest);
		std::printf("%s", table.c_str());
		std::fflush(stdout);
		for (const std::string& line : World::GetMissingAssetLines(manifest))
		{
			DEBUG_INFO("missing %s", line.c_str());
		}
		context.GetGameData().gameState = Core::GameState::QUIT;
	}
	if (Core::CommandLine::HasFlag("--demo"))
	{
		// Demo: the simulated trainer and strap are "remembered" in memory only, so pairing connects them at once and
		// the player's real device choices on disk are never touched.
		Fitness::FitnessManager& fitness = context.GetFitnessManager();
		fitness.SetRegistryPath("");
		fitness.GetRegistry() = Fitness::DeviceRegistry();
		fitness.GetRegistry().Remember(Fitness::DeviceRole::TRAINER,
		                               Fitness::RememberedDevice{"5E:ED:00:00:00:01", "SIM Smart Trainer", "ble"});
		fitness.GetRegistry().Remember(Fitness::DeviceRole::HEART_RATE,
		                               Fitness::RememberedDevice{"5E:ED:00:00:00:02", "SIM Heart Rate", "ble"});
	}
}

#ifdef EDITOR
// Back to nothing loaded, ready to start again.
static void ClearDebugGame(GameContext& context)
{
	App::ResetPlayState(context);
	App::ClearScriptWorld(context);
	// A script loads the world; a restart starts without one.
	World::UnloadWorld(context);
	App::ClearScriptServices(context);
	App::ReleaseGameResources(context);
}

// The editor's Play and Refresh.
static void StartDebugGame(GameContext& context)
{
	ClearDebugGame(context);
	App::LoadScripts(context);
	App::StartPhysics(context);

	DEBUG_INFO("[Start] Wren State");
	context.GetWrenStateManager().Start(context);
}
#endif

static void InitialiseGame(RideContext& context)
{
	FileSystem::SetSaveKey({std::begin(k_saveKey), std::end(k_saveKey)});
	App::MountAssets();
#ifdef EDITOR
	App::OpenEditor(context, AddViews);
	// ClearDebugGame is the editor's Stop.
	context.GetEditor().SetPlayControls({StartDebugGame, ClearDebugGame});
#endif

	context.GetWrenScriptEngine().SetBindings(RegisterBindings);
	App::LoadScripts(context);
#ifndef EDITOR
	App::OpenWindow(context, AddViews);
#endif
	App::ForwardWindowEvents(context);

	const std::string saveDir = App::MountSaves(context);
	StartRide(context, saveDir);
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
	// Before the resource manager: the world renderer holds textures.
	DEBUG_INFO("[Clean Up] World");
	World::UnloadWorld(context);
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
	RideContext context;
	InitialiseGame(context);
	App::RunLoop(context, Frame);
	ExitGame(context);
	App::Shutdown(context);
}
}  // namespace Struktur
