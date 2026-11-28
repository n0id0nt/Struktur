#pragma once

#include "Engine/Dialogue/DialogueServices.h"
#include "Engine/ECS/EcsServices.h"
#include "Engine/Flag/FlagServices.h"
#include "Engine/GameContext.h"
#include "Engine/Physics/PhysicsServices.h"
#include "Engine/Scripting/WrenServices.h"
#include "Engine/UI/UIServices.h"
#include "Engine/World/World2DServices.h"
#ifdef EDITOR
	#include "Debug/Editor/EditorServices.h"
#endif

namespace Struktur
{
// Seven Gods' context: the libraries it uses. Everything the game itself does is in its scripts, so it adds nothing of
// its own. Everything is created in the constructor and destroyed in Shutdown.
class SevenGodsContext final : public GameContext
{
public:
	SevenGodsContext();
	~SevenGodsContext() override;

	void Shutdown() override;

	entt::registry& GetRegistry() const override;
	System::GameObjectManager& GetGameObjectManager() const override;
	System::SystemManager& GetSystemManager() const override;
	Wren::WrenScriptComponentRegistry& GetWrenScriptComponentRegistry() const override;
	Wren::WrenScriptEngine& GetWrenScriptEngine() const override;
	Wren::WrenStateManager& GetWrenStateManager() const override;
	UI::InputGlyphs& GetInputGlyphs() const override;
	UI::UIManager& GetUIManager() const override;
	Renderer::UIRenderer& GetUIRenderer() const override;
	Physics::CollisionLayers& GetCollisionLayers() const override;
	Physics::PhysicsWorld& GetPhysicsWorld() const override;
	World::Camera2D& GetCamera2D() const override;
	Renderer::SpriteBatchRenderer& GetSpriteBatchRenderer() const override;
	Dialogue::DialogueManager& GetDialogueManager() const override;
	Dialogue::DialogueRegistry& GetDialogueRegistry() const override;
	Dialogue::VariableSubstitutionSystem& GetVariableSubstitutionSystem() const override;
	Flag::FlagManager& GetFlagManager() const override;
#ifdef EDITOR
	Debug::Editor& GetEditor() const override;
	Renderer::ImGuiRenderer& GetImGuiRenderer() const override;
#endif

private:
	System::Services m_ecs;
	Wren::Services m_wren;
	UI::Services m_ui;
	Physics::Services m_physics;
	World::Services m_world2d;
	Dialogue::Services m_dialogue;
	Flag::Services m_flag;
#ifdef EDITOR
	Debug::Services m_editor;
#endif
};
}  // namespace Struktur
