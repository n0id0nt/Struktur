#include "SevenGodsContext.h"

namespace Struktur
{
SevenGodsContext::SevenGodsContext() = default;

SevenGodsContext::~SevenGodsContext() = default;

void SevenGodsContext::Shutdown()
{
	StopJobs();
	// The libraries, all before the core services they draw and load with.
	m_world2d.Shutdown();
#ifdef EDITOR
	m_editor.Shutdown();
#endif
	m_ui.Shutdown();
	m_ecs.Shutdown();
	m_physics.Shutdown();
	m_wren.Shutdown();
	m_dialogue.Shutdown();
	m_flag.Shutdown();

	ShutdownCore();
}

entt::registry& SevenGodsContext::GetRegistry() const
{
	ASSERT_MSG(m_ecs.registry.get(), "Registry not initialised");
	return *m_ecs.registry;
}

System::GameObjectManager& SevenGodsContext::GetGameObjectManager() const
{
	ASSERT_MSG(m_ecs.gameObjectManager.get(), "Game Object Manager not initialised");
	return *m_ecs.gameObjectManager;
}

System::SystemManager& SevenGodsContext::GetSystemManager() const
{
	ASSERT_MSG(m_ecs.systemManager.get(), "System Manager not initialised");
	return *m_ecs.systemManager;
}

Wren::WrenScriptComponentRegistry& SevenGodsContext::GetWrenScriptComponentRegistry() const
{
	ASSERT_MSG(m_ecs.scriptComponentRegistry.get(), "Wren Script Component Registry not initialised");
	return *m_ecs.scriptComponentRegistry;
}

Wren::WrenScriptEngine& SevenGodsContext::GetWrenScriptEngine() const
{
	ASSERT_MSG(m_wren.scriptEngine.get(), "Wren Script Engine not initialised");
	return *m_wren.scriptEngine;
}

Wren::WrenStateManager& SevenGodsContext::GetWrenStateManager() const
{
	ASSERT_MSG(m_wren.stateManager.get(), "Wren State Manager not initialised");
	return *m_wren.stateManager;
}

UI::InputGlyphs& SevenGodsContext::GetInputGlyphs() const
{
	ASSERT_MSG(m_ui.inputGlyphs.get(), "InputGlyphs not initialised");
	return *m_ui.inputGlyphs;
}

UI::UIManager& SevenGodsContext::GetUIManager() const
{
	ASSERT_MSG(m_ui.uiManager.get(), "UI Manager not initialised");
	return *m_ui.uiManager;
}

Renderer::UIRenderer& SevenGodsContext::GetUIRenderer() const
{
	ASSERT_MSG(m_ui.uiRenderer.get(), "UIRenderer not initialised");
	return *m_ui.uiRenderer;
}

Physics::CollisionLayers& SevenGodsContext::GetCollisionLayers() const
{
	ASSERT_MSG(m_physics.collisionLayers.get(), "Collision Layers not initialised");
	return *m_physics.collisionLayers;
}

Physics::PhysicsWorld& SevenGodsContext::GetPhysicsWorld() const
{
	ASSERT_MSG(m_physics.physicsWorld.get(), "Physics World not initialised");
	return *m_physics.physicsWorld;
}

World::Camera2D& SevenGodsContext::GetCamera2D() const
{
	ASSERT_MSG(m_world2d.camera.get(), "Camera not initialised");
	return *m_world2d.camera;
}

Renderer::SpriteBatchRenderer& SevenGodsContext::GetSpriteBatchRenderer() const
{
	ASSERT_MSG(m_world2d.spriteRenderer.get(), "Sprite Batch Renderer not initialised");
	return *m_world2d.spriteRenderer;
}

Dialogue::DialogueManager& SevenGodsContext::GetDialogueManager() const
{
	ASSERT_MSG(m_dialogue.dialogueManager.get(), "Dialogue Manager not initialised");
	return *m_dialogue.dialogueManager;
}

Dialogue::DialogueRegistry& SevenGodsContext::GetDialogueRegistry() const
{
	ASSERT_MSG(m_dialogue.dialogueRegistry.get(), "Dialogue Registry not initialised");
	return *m_dialogue.dialogueRegistry;
}

Dialogue::VariableSubstitutionSystem& SevenGodsContext::GetVariableSubstitutionSystem() const
{
	ASSERT_MSG(m_dialogue.variableSubstitutionSystem.get(), "Variable Substitution System not initialised");
	return *m_dialogue.variableSubstitutionSystem;
}

Flag::FlagManager& SevenGodsContext::GetFlagManager() const
{
	ASSERT_MSG(m_flag.flagManager.get(), "Flag Manager not initialised");
	return *m_flag.flagManager;
}

#ifdef EDITOR
Debug::Editor& SevenGodsContext::GetEditor() const
{
	ASSERT_MSG(m_editor.editor.get(), "Editor not initialised");
	return *m_editor.editor;
}

Renderer::ImGuiRenderer& SevenGodsContext::GetImGuiRenderer() const
{
	ASSERT_MSG(m_editor.imGuiRenderer.get(), "ImGuiRenderer not initialised");
	return *m_editor.imGuiRenderer;
}
#endif
}  // namespace Struktur
