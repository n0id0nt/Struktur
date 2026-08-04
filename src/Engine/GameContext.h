#pragma once

#include <memory>
#include <stdexcept>

#include "Debug/Assertions.h"
#include "Engine/Core/GameData.h"
#include "Engine/Core/TimeSystem.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/TextSystems/VariableSubstitutionSystem.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Event/EventManager.h"
#include "Engine/Flag/FlagManager.h"
#include "Engine/Game/Camera.h"
#include "Engine/Input/Input.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Renderer/RenderQueue.h"
#include "Engine/Resource/ResourceManager.h"
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
	GameContext()
	{
		m_input                       = std::make_unique<Input::Input>(0);
		m_gameData                    = std::make_unique<Core::GameData>();
		m_timeSystem                  = std::make_unique<Core::TimeSystem>();
		m_registry                    = std::make_unique<entt::registry>();
		m_resourceManager             = std::make_unique<Resource::ResourceManager>();
		m_systemManager               = std::make_unique<System::SystemManager>();
		m_gameObjectManager           = std::make_unique<System::GameObjectManager>();
		m_camera                      = std::make_unique<GameResource::Camera>();
		m_renderQueue                 = std::make_unique<Renderer::RenderQueue>();
		m_uiManager                   = std::make_unique<UI::UIManager>();
		m_physicsWorld                = std::make_unique<Physics::PhysicsWorld>();
		m_wrenScriptEngine            = std::make_unique<Wren::WrenScriptEngine>();
		m_wrenStateManager            = std::make_unique<Wren::WrenStateManager>();
		m_wrenScriptComponentRegistry = std::make_unique<Wren::WrenScriptComponentRegistry>();
		m_flagManager                 = std::make_unique<Flag::FlagManager>();
		m_dialogueManager             = std::make_unique<Dialogue::DialogueManager>();
		m_dialogueRegistry            = std::make_unique<Dialogue::DialogueRegistry>();
		m_variableSubstitutionSystem  = std::make_unique<Dialogue::VariableSubstitutionSystem>();
		m_eventManager                = std::make_unique<Event::EventManager>();
#ifdef EDITOR
		m_editor = std::make_unique<Debug::Editor>();
#endif
#ifdef DEBUG
		m_profiler = std::make_unique<Debug::Profiler>();
#endif
	}

	Input::Input& GetInput() const
	{
		ASSERT_MSG(m_input.get(), "Input not initialised");
		return *m_input;
	}

	Core::GameData& GetGameData() const
	{
		ASSERT_MSG(m_gameData.get(), "GameData not initialised");
		return *m_gameData;
	}

	Core::TimeSystem& GetTimeSystem() const
	{
		ASSERT_MSG(m_timeSystem.get(), "TimeSystem not initialised");
		return *m_timeSystem;
	}

	entt::registry& GetRegistry() const
	{
		ASSERT_MSG(m_registry.get(), "Registry not initialised");
		return *m_registry;
	}

	Resource::ResourceManager& GetResourceManager() const
	{
		ASSERT_MSG(m_resourceManager.get(), "Resource Manager not initialised");
		return *m_resourceManager;
	}

	System::SystemManager& GetSystemManager() const
	{
		ASSERT_MSG(m_systemManager.get(), "System Manager not initialised");
		return *m_systemManager;
	}

	System::GameObjectManager& GetGameObjectManager() const
	{
		ASSERT_MSG(m_gameObjectManager.get(), "Game Object Manager not initialised");
		return *m_gameObjectManager;
	}

	Physics::PhysicsWorld& GetPhysicsWorld() const
	{
		ASSERT_MSG(m_physicsWorld.get(), "Physics World not initialised");
		return *m_physicsWorld;
	}

	GameResource::Camera& GetCamera() const
	{
		ASSERT_MSG(m_camera.get(), "Camera not initialised");
		return *m_camera;
	}

	Renderer::RenderQueue& GetRenderQueue() const
	{
		ASSERT_MSG(m_renderQueue.get(), "Render Queue not initialised");
		return *m_renderQueue;
	}

	UI::UIManager& GetUIManager() const
	{
		ASSERT_MSG(m_uiManager.get(), "UI Manager not initialised");
		return *m_uiManager;
	}

	Wren::WrenScriptEngine& GetWrenScriptEngine() const
	{
		ASSERT_MSG(m_wrenScriptEngine.get(), "Wren Script Engine not initialised");
		return *m_wrenScriptEngine;
	}

	Wren::WrenStateManager& GetWrenStateManager() const
	{
		ASSERT_MSG(m_wrenStateManager.get(), "Wren State Manager not initialised");
		return *m_wrenStateManager;
	}

	Wren::WrenScriptComponentRegistry& GetWrenScriptComponentRegistry() const
	{
		ASSERT_MSG(m_wrenScriptComponentRegistry.get(), "Wren State Manager not initialised");
		return *m_wrenScriptComponentRegistry;
	}

	Flag::FlagManager& GetFlagManager() const
	{
		ASSERT_MSG(m_flagManager.get(), "Flag Manager not initialised");
		return *m_flagManager;
	}

	Dialogue::DialogueManager& GetDialogueManager() const
	{
		ASSERT_MSG(m_dialogueManager.get(), "Dialogue Manager not initialised");
		return *m_dialogueManager;
	}

	Dialogue::DialogueRegistry& GetDialogueRegistry() const
	{
		ASSERT_MSG(m_dialogueRegistry.get(), "Dialogue Registry not initialised");
		return *m_dialogueRegistry;
	}

	Dialogue::VariableSubstitutionSystem& GetVariableSubstitutionSystem() const
	{
		ASSERT_MSG(m_variableSubstitutionSystem.get(), "Variable Substitution System not initialised");
		return *m_variableSubstitutionSystem;
	}

	Event::EventManager& GetEventManager() const
	{
		ASSERT_MSG(m_eventManager.get(), "Event Manager not initialised");
		return *m_eventManager;
	}

#ifdef EDITOR
	Debug::Editor& GetEditor() const
	{
		ASSERT_MSG(m_editor.get(), "Editor not initialised");
		return *m_editor;
	}
#endif

#ifdef DEBUG
	Debug::Profiler& GetProfiler() const
	{
		ASSERT_MSG(m_profiler.get(), "Profiler not initialised");
		return *m_profiler;
	}
#endif

   private:
	std::unique_ptr<Core::GameData> m_gameData;
	std::unique_ptr<Core::TimeSystem> m_timeSystem;
	std::unique_ptr<Input::Input> m_input;
	std::unique_ptr<entt::registry> m_registry;
	std::unique_ptr<Resource::ResourceManager> m_resourceManager;
	std::unique_ptr<System::SystemManager> m_systemManager;
	std::unique_ptr<System::GameObjectManager> m_gameObjectManager;
	std::unique_ptr<Physics::PhysicsWorld> m_physicsWorld;
	std::unique_ptr<GameResource::Camera> m_camera;
	std::unique_ptr<Renderer::RenderQueue> m_renderQueue;
	std::unique_ptr<UI::UIManager> m_uiManager;
	std::unique_ptr<Wren::WrenScriptEngine> m_wrenScriptEngine;
	std::unique_ptr<Wren::WrenStateManager> m_wrenStateManager;
	std::unique_ptr<Wren::WrenScriptComponentRegistry> m_wrenScriptComponentRegistry;
	std::unique_ptr<Flag::FlagManager> m_flagManager;
	std::unique_ptr<Dialogue::DialogueManager> m_dialogueManager;
	std::unique_ptr<Dialogue::DialogueRegistry> m_dialogueRegistry;
	std::unique_ptr<Dialogue::VariableSubstitutionSystem> m_variableSubstitutionSystem;
	std::unique_ptr<Event::EventManager> m_eventManager;
#ifdef EDITOR
	std::unique_ptr<Debug::Editor> m_editor;
#endif
#ifdef DEBUG
	std::unique_ptr<Debug::Profiler> m_profiler;
#endif
};
}  // namespace Struktur
