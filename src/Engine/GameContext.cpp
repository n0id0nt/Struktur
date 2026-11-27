#include "GameContext.h"

#include <cstdlib>

#include "Engine/Core/CommandLine.h"
#include "Engine/Fitness/Ble/BleCentralFactory.h"
#include "Engine/Fitness/Ble/SimulatedBleCentral.h"

namespace Struktur
{
// --demo on the command line, or STRUKTUR_FITNESS_SIMULATE=1, swaps Bluetooth for a fake trainer, heart rate strap and
// power meter driven by a synthetic rider, so every fitness screen can be worked on without hardware.
static std::unique_ptr<Fitness::IBleCentral> CreateFitnessCentral()
{
	const char* simulate = std::getenv("STRUKTUR_FITNESS_SIMULATE");
	if (Core::CommandLine::HasFlag("--demo"))
	{
		DEBUG_INFO("--demo - using simulated fitness devices and a synthetic rider");
		return std::make_unique<Fitness::SimulatedBleCentral>();
	}
	if (simulate != nullptr && simulate[0] != '\0' && simulate[0] != '0')
	{
		DEBUG_INFO("STRUKTUR_FITNESS_SIMULATE is set - using simulated fitness devices");
		return std::make_unique<Fitness::SimulatedBleCentral>();
	}
	return Fitness::CreatePlatformBleCentral();
}

// --threads=N picks the worker count (0 = no worker threads, everything runs on the main thread); otherwise one per
// core minus the main thread. Builds with STRUKTUR_THREADS off (web by default) always get zero.
static int RequestedWorkerCount()
{
	const std::string prefix = "--threads=";
	for (const std::string& argument : Core::CommandLine::GetArguments())
	{
		if (argument.rfind(prefix, 0) == 0)
		{
			return std::atoi(argument.c_str() + prefix.size());
		}
	}
	return -1;
}

GameContext::GameContext()
{
	m_jobSystem      = std::make_unique<Jobs::JobSystem>(Jobs::JobSystemConfig{RequestedWorkerCount()});
	m_frameScheduler = std::make_unique<Jobs::FrameScheduler>(*m_jobSystem);
	m_camera           = std::make_unique<World::Camera>();
	m_collisionLayers  = std::make_unique<Physics::CollisionLayers>();
	m_dialogueManager  = std::make_unique<Dialogue::DialogueManager>();
	m_dialogueRegistry = std::make_unique<Dialogue::DialogueRegistry>();
#ifdef EDITOR
	m_editor = std::make_unique<Debug::Editor>();
#endif
	m_eventManager      = std::make_unique<Event::EventManager>();
	// Idle until the game asks it to scan or connect (see docs/SmartTrainer.md) - constructing it never touches the
	// radio. Non-Windows platforms get a backend that reports Bluetooth as unavailable.
	m_fitnessManager = std::make_unique<Fitness::FitnessManager>(CreateFitnessCentral());
	m_rideService    = std::make_unique<RideIntegration::RideService>(m_jobSystem.get());
	// Both empty until a script loads them (World.load).
	m_world             = std::make_unique<World::World>();
	m_rideWorldRenderer = std::make_unique<Renderer::RideWorldRenderer>();
	m_flagManager       = std::make_unique<Flag::FlagManager>();
	m_gameData          = std::make_unique<Core::GameData>();
	m_gameObjectManager = std::make_unique<System::GameObjectManager>();
	m_graphicsDevice    = std::make_unique<Renderer::GraphicsDevice>();
#ifdef EDITOR
	m_imGuiRenderer = std::make_unique<Renderer::ImGuiRenderer>();
#endif
	m_input               = std::make_unique<Input::Input>(0);
	m_inputGlyphs         = std::make_unique<UI::InputGlyphs>();
	m_localizationManager = std::make_unique<Localization::LocalizationManager>();
	m_mixer               = std::make_unique<Audio::Mixer>();
	m_physicsWorld = std::make_unique<Physics::PhysicsWorld>();
#ifdef DEBUG
	m_profiler = std::make_unique<Debug::Profiler>();
#endif
	m_registry                    = std::make_unique<entt::registry>();
	m_resourceManager             = std::make_unique<Resource::ResourceManager>();
	m_systemManager               = std::make_unique<System::SystemManager>();
	m_timeSystem                  = std::make_unique<Core::TimeSystem>();
	m_uiManager                   = std::make_unique<UI::UIManager>();
	m_uiRenderer                  = std::make_unique<Renderer::UIRenderer>();
	m_variableSubstitutionSystem  = std::make_unique<Dialogue::VariableSubstitutionSystem>();
	m_window                      = std::make_unique<Platform::Window>();
	m_worldRenderer               = std::make_unique<Renderer::WorldRenderer>();
	m_wrenScriptComponentRegistry = std::make_unique<Wren::WrenScriptComponentRegistry>();
	m_wrenScriptEngine            = std::make_unique<Wren::WrenScriptEngine>();
	m_wrenStateManager            = std::make_unique<Wren::WrenStateManager>();
}

void GameContext::Shutdown()
{
	// Jobs first: running jobs finish, queued ones and their main-thread follow-ups are dropped, so nothing below is
	// torn down while a worker might still be using it.
	m_frameScheduler.reset();
	m_jobSystem->Shutdown();
	// Every member below (other than the graphics device itself) must be torn down before
	// m_graphicsDevice->Shutdown() runs - bgfx::destroy() is invalid once bgfx has shut down. Resetting
	// everything else first, in any order, means declaration order above no longer has to encode this.
	// First, while everything else is still alive: a graceful trainer stop (FTMS Reset, then disconnect) so a
	// single-connection trainer is released for other apps the moment the game exits.
	// An unfinished ride is saved (autosave already keeps it current) before the trainer is released.
	if (m_rideService && m_rideService->IsActive())
	{
		m_rideService->Finish(m_timeSystem ? m_timeSystem->unscaledTime : 0.0);
	}
	m_rideService.reset();
	// GPU resources first (bgfx is still up), then the world, which waits for its (by now complete) chunk builds.
	m_rideWorldRenderer->Shutdown();
	m_rideWorldRenderer.reset();
	m_world.reset();
	m_fitnessManager.reset();
	m_camera.reset();
	m_collisionLayers.reset();
	m_dialogueManager.reset();
	m_dialogueRegistry.reset();
#ifdef EDITOR
	m_editor.reset();
#endif
	m_eventManager.reset();
	m_flagManager.reset();
	m_gameData.reset();
	m_gameObjectManager.reset();
#ifdef EDITOR
	m_imGuiRenderer->Shutdown();
	m_imGuiRenderer.reset();
#endif
	m_input.reset();
	m_inputGlyphs.reset();  // holds texture ResourcePtrs - must go before m_resourceManager
	m_localizationManager.reset();
	m_mixer.reset();
	m_physicsWorld.reset();
#ifdef DEBUG
	m_profiler.reset();
#endif
	m_registry.reset();
	m_resourceManager.reset();
	m_systemManager.reset();
	m_timeSystem.reset();
	m_uiManager.reset();
	m_uiRenderer->Shutdown();
	m_uiRenderer.reset();
	m_variableSubstitutionSystem.reset();
	m_worldRenderer->Shutdown();
	m_worldRenderer.reset();
	m_wrenScriptComponentRegistry.reset();
	m_wrenScriptEngine.reset();
	m_wrenStateManager.reset();

	m_graphicsDevice->Shutdown();
	m_graphicsDevice.reset();
	m_window.reset();
	// Last: objects torn down above (ride world, ride session) may still wait on their - by now complete - tasks.
	m_jobSystem.reset();
}

World::Camera& GameContext::GetCamera() const
{
	ASSERT_MSG(m_camera.get(), "Camera not initialised");
	return *m_camera;
}

Physics::CollisionLayers& GameContext::GetCollisionLayers() const
{
	ASSERT_MSG(m_collisionLayers.get(), "Collision Layers not initialised");
	return *m_collisionLayers;
}

Dialogue::DialogueManager& GameContext::GetDialogueManager() const
{
	ASSERT_MSG(m_dialogueManager.get(), "Dialogue Manager not initialised");
	return *m_dialogueManager;
}

Dialogue::DialogueRegistry& GameContext::GetDialogueRegistry() const
{
	ASSERT_MSG(m_dialogueRegistry.get(), "Dialogue Registry not initialised");
	return *m_dialogueRegistry;
}

#ifdef EDITOR
Debug::Editor& GameContext::GetEditor() const
{
	ASSERT_MSG(m_editor.get(), "Editor not initialised");
	return *m_editor;
}
#endif

Fitness::FitnessManager& GameContext::GetFitnessManager() const
{
	ASSERT_MSG(m_fitnessManager.get(), "Fitness Manager not initialised");
	return *m_fitnessManager;
}

RideIntegration::RideService& GameContext::GetRideService() const
{
	ASSERT_MSG(m_rideService.get(), "Ride Service not initialised");
	return *m_rideService;
}

Event::EventManager& GameContext::GetEventManager() const
{
	ASSERT_MSG(m_eventManager.get(), "Event Manager not initialised");
	return *m_eventManager;
}

Flag::FlagManager& GameContext::GetFlagManager() const
{
	ASSERT_MSG(m_flagManager.get(), "Flag Manager not initialised");
	return *m_flagManager;
}

Core::GameData& GameContext::GetGameData() const
{
	ASSERT_MSG(m_gameData.get(), "GameData not initialised");
	return *m_gameData;
}

System::GameObjectManager& GameContext::GetGameObjectManager() const
{
	ASSERT_MSG(m_gameObjectManager.get(), "Game Object Manager not initialised");
	return *m_gameObjectManager;
}

Renderer::GraphicsDevice& GameContext::GetGraphicsDevice() const
{
	ASSERT_MSG(m_graphicsDevice.get(), "GraphicsDevice not initialised");
	return *m_graphicsDevice;
}

#if defined(EDITOR)
Renderer::ImGuiRenderer& GameContext::GetImGuiRenderer() const
{
	ASSERT_MSG(m_imGuiRenderer.get(), "ImGuiRenderer not initialised");
	return *m_imGuiRenderer;
}
#endif

Input::Input& GameContext::GetInput() const
{
	ASSERT_MSG(m_input.get(), "Input not initialised");
	return *m_input;
}

Localization::LocalizationManager& GameContext::GetLocalizationManager() const
{
	ASSERT_MSG(m_localizationManager.get(), "Localization Manager not initialised");
	return *m_localizationManager;
}

Audio::Mixer& GameContext::GetMixer() const
{
	ASSERT_MSG(m_mixer.get(), "Mixer not initialised");
	return *m_mixer;
}

Physics::PhysicsWorld& GameContext::GetPhysicsWorld() const
{
	ASSERT_MSG(m_physicsWorld.get(), "Physics World not initialised");
	return *m_physicsWorld;
}

#ifdef DEBUG
Debug::Profiler& GameContext::GetProfiler() const
{
	ASSERT_MSG(m_profiler.get(), "Profiler not initialised");
	return *m_profiler;
}
#endif

entt::registry& GameContext::GetRegistry() const
{
	ASSERT_MSG(m_registry.get(), "Registry not initialised");
	return *m_registry;
}

Jobs::JobSystem& GameContext::GetJobSystem() const
{
	ASSERT_MSG(m_jobSystem.get(), "Job System not initialised");
	return *m_jobSystem;
}

Jobs::FrameScheduler& GameContext::GetFrameScheduler() const
{
	ASSERT_MSG(m_frameScheduler.get(), "Frame Scheduler not initialised");
	return *m_frameScheduler;
}

World::World& GameContext::GetWorld() const
{
	ASSERT_MSG(m_world.get(), "World not initialised");
	return *m_world;
}

Renderer::RideWorldRenderer& GameContext::GetRideWorldRenderer() const
{
	ASSERT_MSG(m_rideWorldRenderer.get(), "Ride World Renderer not initialised");
	return *m_rideWorldRenderer;
}

Resource::ResourceManager& GameContext::GetResourceManager() const
{
	ASSERT_MSG(m_resourceManager.get(), "Resource Manager not initialised");
	return *m_resourceManager;
}

System::SystemManager& GameContext::GetSystemManager() const
{
	ASSERT_MSG(m_systemManager.get(), "System Manager not initialised");
	return *m_systemManager;
}

Core::TimeSystem& GameContext::GetTimeSystem() const
{
	ASSERT_MSG(m_timeSystem.get(), "TimeSystem not initialised");
	return *m_timeSystem;
}

UI::InputGlyphs& GameContext::GetInputGlyphs() const
{
	ASSERT_MSG(m_inputGlyphs.get(), "InputGlyphs not initialised");
	return *m_inputGlyphs;
}

UI::UIManager& GameContext::GetUIManager() const
{
	ASSERT_MSG(m_uiManager.get(), "UI Manager not initialised");
	return *m_uiManager;
}

Renderer::UIRenderer& GameContext::GetUIRenderer() const
{
	ASSERT_MSG(m_uiRenderer.get(), "UIRenderer not initialised");
	return *m_uiRenderer;
}

Dialogue::VariableSubstitutionSystem& GameContext::GetVariableSubstitutionSystem() const
{
	ASSERT_MSG(m_variableSubstitutionSystem.get(), "Variable Substitution System not initialised");
	return *m_variableSubstitutionSystem;
}

Platform::Window& GameContext::GetWindow() const
{
	ASSERT_MSG(m_window.get(), "Window not initialised");
	return *m_window;
}

Renderer::WorldRenderer& GameContext::GetWorldRenderer() const
{
	ASSERT_MSG(m_worldRenderer.get(), "World Renderer not initialised");
	return *m_worldRenderer;
}

Wren::WrenScriptComponentRegistry& GameContext::GetWrenScriptComponentRegistry() const
{
	ASSERT_MSG(m_wrenScriptComponentRegistry.get(), "Wren State Manager not initialised");
	return *m_wrenScriptComponentRegistry;
}

Wren::WrenScriptEngine& GameContext::GetWrenScriptEngine() const
{
	ASSERT_MSG(m_wrenScriptEngine.get(), "Wren Script Engine not initialised");
	return *m_wrenScriptEngine;
}

Wren::WrenStateManager& GameContext::GetWrenStateManager() const
{
	ASSERT_MSG(m_wrenStateManager.get(), "Wren State Manager not initialised");
	return *m_wrenStateManager;
}
}  // namespace Struktur
