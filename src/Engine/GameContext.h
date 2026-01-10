#pragma once

#include <memory>
#include <stdexcept>
#include "entt/entt.hpp"

#include "Engine/Core/Input.h"
#include "Engine/Core/GameData.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Game/Camera.h"
#include "Engine/UI/UIManager.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"
#include "Engine/Scripting/WrenScriptComponentRegistry.h"

#include "Debug/Assertions.h"
#ifdef EDITOR
#include "Debug/Editor/Editor.h"
#endif

namespace Struktur
{
	using Inventory = std::vector<std::string>;
	class GameContext
	{
	public:
		GameContext()
		{
			m_input = std::make_unique<Core::Input>(0);
			m_gameData = std::make_unique<Core::GameData>();
			m_registry = std::make_unique<entt::registry>();
			m_resourceManager = std::make_unique<Resource::ResourceManager>();
			m_systemManager = std::make_unique<System::SystemManager>();
			m_gameObjectManager = std::make_unique<System::GameObjectManager>();
			m_camera = std::make_unique<GameResource::Camera>();
			m_uiManger = std::make_unique<UI::UIManager>();
			m_inventory = std::make_unique<Inventory>();
			m_physicsWorld = std::make_unique<Physics::PhysicsWorld>();
			m_wrenScriptEngine = std::make_unique<Wren::WrenScriptEngine>();
			m_wrenStateManager = std::make_unique<Wren::WrenStateManager>();
			m_wrenScriptComponentRegistry = std::make_unique<Wren::WrenScriptComponentRegistry>();
#ifdef EDITOR
			m_editor = std::make_unique<Debug::Editor>();
#endif
		}

		Core::Input& GetInput() const
		{
			ASSERT_MSG(m_input.get(), "Input not initialised");
			return *m_input;
		}

		Core::GameData& GetGameData() const
		{
			ASSERT_MSG(m_gameData.get(), "GameData not initialised");
			return *m_gameData;
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

		UI::UIManager& GetUIManager() const
		{
			ASSERT_MSG(m_uiManger.get(), "UI Manager not initialised");
			return *m_uiManger;
		}

		Inventory& GetInventory() const
		{
			ASSERT_MSG(m_inventory.get(), "Inventory not initialised");
			return *m_inventory;
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

#ifdef EDITOR
		Debug::Editor& GetEditor() const
		{
			ASSERT_MSG(m_editor.get(), "Editor not initialised");
			return *m_editor;
		}
#endif

	private:
		std::unique_ptr<Core::GameData> m_gameData;
		std::unique_ptr<Core::Input> m_input;
		std::unique_ptr<entt::registry> m_registry;
		std::unique_ptr<Resource::ResourceManager> m_resourceManager;
		std::unique_ptr<System::SystemManager> m_systemManager;
		std::unique_ptr<System::GameObjectManager> m_gameObjectManager;
		std::unique_ptr<Physics::PhysicsWorld> m_physicsWorld;
		std::unique_ptr<GameResource::Camera> m_camera;
		std::unique_ptr<UI::UIManager> m_uiManger;
		std::unique_ptr<Inventory> m_inventory;
		std::unique_ptr<Wren::WrenScriptEngine> m_wrenScriptEngine;
		std::unique_ptr<Wren::WrenStateManager> m_wrenStateManager;
		std::unique_ptr<Wren::WrenScriptComponentRegistry> m_wrenScriptComponentRegistry;
#ifdef EDITOR
		std::unique_ptr<Debug::Editor> m_editor;
#endif
	};
}
