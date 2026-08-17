#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "Debug/Assertions.h"
#include "Engine/Audio/Mixer.h"
#include "Engine/Core/GameData.h"
#include "Engine/Core/TimeSystem.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/TextSystems/VariableSubstitutionSystem.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Event/EventManager.h"
#include "Engine/Flag/FlagManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Physics/CollisionLayers.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Renderer/WorldRenderer.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/World/Camera.h"
#if defined(EDITOR)
	#include "Engine/Renderer/ImGuiRenderer.h"
#endif
#include "Engine/Scripting/WrenScriptComponentRegistry.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"
#include "Engine/UI/UIManager.h"
#include "entt/entt.hpp"
#ifdef EDITOR
	#include "Debug/Editor/Editor.h"
#endif
#ifdef DEBUG
	#include "Debug/Profiling/Profiler.h"
#endif

namespace Struktur
{
class GameContext
{
public:
	GameContext();

	void Shutdown();

	World::Camera& GetCamera() const;
	Physics::CollisionLayers& GetCollisionLayers() const;
	Dialogue::DialogueManager& GetDialogueManager() const;
	Dialogue::DialogueRegistry& GetDialogueRegistry() const;
#ifdef EDITOR
	Debug::Editor& GetEditor() const;
#endif
	Event::EventManager& GetEventManager() const;
	Flag::FlagManager& GetFlagManager() const;
	Core::GameData& GetGameData() const;
	System::GameObjectManager& GetGameObjectManager() const;
	Renderer::GraphicsDevice& GetGraphicsDevice() const;
#ifdef EDITOR
	Renderer::ImGuiRenderer& GetImGuiRenderer() const;
#endif
	Input::Input& GetInput() const;
	Audio::Mixer& GetMixer() const;
	Physics::PhysicsWorld& GetPhysicsWorld() const;
#ifdef DEBUG
	Debug::Profiler& GetProfiler() const;
#endif
	entt::registry& GetRegistry() const;
	Resource::ResourceManager& GetResourceManager() const;
	System::SystemManager& GetSystemManager() const;
	Core::TimeSystem& GetTimeSystem() const;
	UI::UIManager& GetUIManager() const;
	Renderer::UIRenderer& GetUIRenderer() const;
	Dialogue::VariableSubstitutionSystem& GetVariableSubstitutionSystem() const;
	Platform::Window& GetWindow() const;
	Renderer::WorldRenderer& GetWorldRenderer() const;
	Wren::WrenScriptComponentRegistry& GetWrenScriptComponentRegistry() const;
	Wren::WrenScriptEngine& GetWrenScriptEngine() const;
	Wren::WrenStateManager& GetWrenStateManager() const;

private:
	std::unique_ptr<World::Camera> m_camera;
	std::unique_ptr<Physics::CollisionLayers> m_collisionLayers;
	std::unique_ptr<Dialogue::DialogueManager> m_dialogueManager;
	std::unique_ptr<Dialogue::DialogueRegistry> m_dialogueRegistry;
#ifdef EDITOR
	std::unique_ptr<Debug::Editor> m_editor;
#endif
	std::unique_ptr<Event::EventManager> m_eventManager;
	std::unique_ptr<Flag::FlagManager> m_flagManager;
	std::unique_ptr<Core::GameData> m_gameData;
	std::unique_ptr<System::GameObjectManager> m_gameObjectManager;
	std::unique_ptr<Renderer::GraphicsDevice> m_graphicsDevice;
#ifdef EDITOR
	std::unique_ptr<Renderer::ImGuiRenderer> m_imGuiRenderer;
#endif
	std::unique_ptr<Input::Input> m_input;
	std::unique_ptr<Audio::Mixer> m_mixer;
	std::unique_ptr<Physics::PhysicsWorld> m_physicsWorld;
#ifdef DEBUG
	std::unique_ptr<Debug::Profiler> m_profiler;
#endif
	std::unique_ptr<entt::registry> m_registry;
	std::unique_ptr<Resource::ResourceManager> m_resourceManager;
	std::unique_ptr<System::SystemManager> m_systemManager;
	std::unique_ptr<Core::TimeSystem> m_timeSystem;
	std::unique_ptr<UI::UIManager> m_uiManager;
	std::unique_ptr<Renderer::UIRenderer> m_uiRenderer;
	std::unique_ptr<Dialogue::VariableSubstitutionSystem> m_variableSubstitutionSystem;
	std::unique_ptr<Platform::Window> m_window;
	std::unique_ptr<Renderer::WorldRenderer> m_worldRenderer;
	std::unique_ptr<Wren::WrenScriptComponentRegistry> m_wrenScriptComponentRegistry;
	std::unique_ptr<Wren::WrenScriptEngine> m_wrenScriptEngine;
	std::unique_ptr<Wren::WrenStateManager> m_wrenStateManager;
};
}  // namespace Struktur
