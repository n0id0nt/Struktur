#include "RideContext.h"

#include "Engine/Renderer/WorldRenderer.h"
#include "Engine/RideIntegration/RideService.h"
#include "Engine/World/Camera.h"
#include "Engine/World/World.h"

namespace Struktur
{
RideContext::RideContext()
{
	m_camera      = std::make_unique<World::Camera>();
	m_rideService = std::make_unique<RideIntegration::RideService>(&GetJobSystem());
	// Both empty until a script loads them (World.load).
	m_world         = std::make_unique<World::World>();
	m_worldRenderer = std::make_unique<Renderer::WorldRenderer>();
}

RideContext::~RideContext() = default;

void RideContext::Shutdown()
{
	StopJobs();
	// First, while everything else is still alive: a graceful trainer stop (FTMS Reset, then disconnect) so a
	// single-connection trainer is released for other apps the moment the game exits.
	// An unfinished ride is saved (autosave already keeps it current) before the trainer is released.
	if (m_rideService && m_rideService->IsActive())
	{
		m_rideService->Finish(GetTimeSystem().unscaledTime);
	}
	m_rideService.reset();
	// GPU resources first (bgfx is still up), then the world, which waits for its (by now complete) chunk builds.
	m_worldRenderer->Shutdown();
	m_worldRenderer.reset();
	m_world.reset();
	m_camera.reset();

	// The libraries, all before the core services they draw and load with.
	m_fitness.Shutdown();
#ifdef STRUKTUR_DIALOGUE
	m_dialogue.Shutdown();
#endif
#ifdef EDITOR
	m_editor.Shutdown();
#endif
#ifdef STRUKTUR_FLAG
	m_flag.Shutdown();
#endif
	m_ui.Shutdown();
#ifdef STRUKTUR_PHYSICS
	m_physics.Shutdown();
#endif
	m_ecs.Shutdown();
	m_wren.Shutdown();

	ShutdownCore();
}

RideIntegration::RideService& RideContext::GetRideService() const
{
	ASSERT_MSG(m_rideService.get(), "Ride Service not initialised");
	return *m_rideService;
}

World::World& RideContext::GetWorld() const
{
	ASSERT_MSG(m_world.get(), "World not initialised");
	return *m_world;
}

World::Camera& RideContext::GetCamera() const
{
	ASSERT_MSG(m_camera.get(), "Camera not initialised");
	return *m_camera;
}

Renderer::WorldRenderer& RideContext::GetWorldRenderer() const
{
	ASSERT_MSG(m_worldRenderer.get(), "World Renderer not initialised");
	return *m_worldRenderer;
}

entt::registry& RideContext::GetRegistry() const
{
	ASSERT_MSG(m_ecs.registry.get(), "Registry not initialised");
	return *m_ecs.registry;
}

System::GameObjectManager& RideContext::GetGameObjectManager() const
{
	ASSERT_MSG(m_ecs.gameObjectManager.get(), "Game Object Manager not initialised");
	return *m_ecs.gameObjectManager;
}

System::SystemManager& RideContext::GetSystemManager() const
{
	ASSERT_MSG(m_ecs.systemManager.get(), "System Manager not initialised");
	return *m_ecs.systemManager;
}

Wren::WrenScriptComponentRegistry& RideContext::GetWrenScriptComponentRegistry() const
{
	ASSERT_MSG(m_ecs.scriptComponentRegistry.get(), "Wren Script Component Registry not initialised");
	return *m_ecs.scriptComponentRegistry;
}

Wren::WrenScriptEngine& RideContext::GetWrenScriptEngine() const
{
	ASSERT_MSG(m_wren.scriptEngine.get(), "Wren Script Engine not initialised");
	return *m_wren.scriptEngine;
}

Wren::WrenStateManager& RideContext::GetWrenStateManager() const
{
	ASSERT_MSG(m_wren.stateManager.get(), "Wren State Manager not initialised");
	return *m_wren.stateManager;
}

UI::InputGlyphs& RideContext::GetInputGlyphs() const
{
	ASSERT_MSG(m_ui.inputGlyphs.get(), "InputGlyphs not initialised");
	return *m_ui.inputGlyphs;
}

UI::UIManager& RideContext::GetUIManager() const
{
	ASSERT_MSG(m_ui.uiManager.get(), "UI Manager not initialised");
	return *m_ui.uiManager;
}

Renderer::UIRenderer& RideContext::GetUIRenderer() const
{
	ASSERT_MSG(m_ui.uiRenderer.get(), "UIRenderer not initialised");
	return *m_ui.uiRenderer;
}

Fitness::FitnessManager& RideContext::GetFitnessManager() const
{
	ASSERT_MSG(m_fitness.fitnessManager.get(), "Fitness Manager not initialised");
	return *m_fitness.fitnessManager;
}

#ifdef STRUKTUR_PHYSICS
Physics::CollisionLayers& RideContext::GetCollisionLayers() const
{
	ASSERT_MSG(m_physics.collisionLayers.get(), "Collision Layers not initialised");
	return *m_physics.collisionLayers;
}

Physics::PhysicsWorld& RideContext::GetPhysicsWorld() const
{
	ASSERT_MSG(m_physics.physicsWorld.get(), "Physics World not initialised");
	return *m_physics.physicsWorld;
}
#endif

#ifdef STRUKTUR_DIALOGUE
Dialogue::DialogueManager& RideContext::GetDialogueManager() const
{
	ASSERT_MSG(m_dialogue.dialogueManager.get(), "Dialogue Manager not initialised");
	return *m_dialogue.dialogueManager;
}

Dialogue::DialogueRegistry& RideContext::GetDialogueRegistry() const
{
	ASSERT_MSG(m_dialogue.dialogueRegistry.get(), "Dialogue Registry not initialised");
	return *m_dialogue.dialogueRegistry;
}

Dialogue::VariableSubstitutionSystem& RideContext::GetVariableSubstitutionSystem() const
{
	ASSERT_MSG(m_dialogue.variableSubstitutionSystem.get(), "Variable Substitution System not initialised");
	return *m_dialogue.variableSubstitutionSystem;
}
#endif

#ifdef STRUKTUR_FLAG
Flag::FlagManager& RideContext::GetFlagManager() const
{
	ASSERT_MSG(m_flag.flagManager.get(), "Flag Manager not initialised");
	return *m_flag.flagManager;
}
#endif

#ifdef EDITOR
Debug::Editor& RideContext::GetEditor() const
{
	ASSERT_MSG(m_editor.editor.get(), "Editor not initialised");
	return *m_editor.editor;
}

Renderer::ImGuiRenderer& RideContext::GetImGuiRenderer() const
{
	ASSERT_MSG(m_editor.imGuiRenderer.get(), "ImGuiRenderer not initialised");
	return *m_editor.imGuiRenderer;
}
#endif
}  // namespace Struktur
