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
#include "Engine/Game/Camera.h"
#include "Engine/Input/Input.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Renderer/WorldRenderer.h"
#include "Engine/Resource/ResourceManager.h"
#if !defined(PLATFORM_WEB)
	#include "Engine/Platform/Window.h"
	#include "Engine/Renderer/GraphicsDevice.h"
	#include "Engine/Renderer/UIRenderer.h"
	#if defined(EDITOR)
		#include "Engine/Renderer/ImGuiRenderer.h"
	#endif
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

	Input::Input& GetInput() const;
	Core::GameData& GetGameData() const;
	Core::TimeSystem& GetTimeSystem() const;
	entt::registry& GetRegistry() const;
	Resource::ResourceManager& GetResourceManager() const;
	System::SystemManager& GetSystemManager() const;
	System::GameObjectManager& GetGameObjectManager() const;
	Physics::PhysicsWorld& GetPhysicsWorld() const;
	GameResource::Camera& GetCamera() const;
	Renderer::WorldRenderer& GetWorldRenderer() const;
	UI::UIManager& GetUIManager() const;
	Wren::WrenScriptEngine& GetWrenScriptEngine() const;
	Wren::WrenStateManager& GetWrenStateManager() const;
	Wren::WrenScriptComponentRegistry& GetWrenScriptComponentRegistry() const;
	Flag::FlagManager& GetFlagManager() const;
	Dialogue::DialogueManager& GetDialogueManager() const;
	Dialogue::DialogueRegistry& GetDialogueRegistry() const;
	Dialogue::VariableSubstitutionSystem& GetVariableSubstitutionSystem() const;
	Event::EventManager& GetEventManager() const;
	Audio::Mixer& GetMixer() const;

#if !defined(PLATFORM_WEB)
	// Deferred until the desired window size is known (see Game.cpp's InitialiseGame) - unlike every other
	// subsystem above, these aren't ready immediately after construction.
	void InitialiseGraphics(int width, int height, const std::string& title, bool resizable);
	Platform::Window& GetWindow() const;
	Renderer::GraphicsDevice& GetGraphicsDevice() const;

	// Deferred until GraphicsDevice (and therefore bgfx) exists - call right after InitialiseGraphics(). Needed
	// on both platforms (in-game UI isn't editor-only), unlike ImGuiRenderer below.
	void InitialiseUIRenderer();
	Renderer::UIRenderer& GetUIRenderer() const;

	#if defined(EDITOR)
	// Deferred further still - needs an ImGui context (and its font atlas configured) to already exist,
	// which only happens once Game.cpp has called ImGui::CreateContext()/ImGui_ImplSDL3_InitForOther().
	void InitialiseImGuiRenderer();
	Renderer::ImGuiRenderer& GetImGuiRenderer() const;
	#endif
#endif

#ifdef EDITOR
	Debug::Editor& GetEditor() const;
#endif

#ifdef DEBUG
	Debug::Profiler& GetProfiler() const;
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
	std::unique_ptr<Renderer::WorldRenderer> m_worldRenderer;
	std::unique_ptr<UI::UIManager> m_uiManager;
	std::unique_ptr<Wren::WrenScriptEngine> m_wrenScriptEngine;
	std::unique_ptr<Wren::WrenStateManager> m_wrenStateManager;
	std::unique_ptr<Wren::WrenScriptComponentRegistry> m_wrenScriptComponentRegistry;
	std::unique_ptr<Flag::FlagManager> m_flagManager;
	std::unique_ptr<Dialogue::DialogueManager> m_dialogueManager;
	std::unique_ptr<Dialogue::DialogueRegistry> m_dialogueRegistry;
	std::unique_ptr<Dialogue::VariableSubstitutionSystem> m_variableSubstitutionSystem;
	std::unique_ptr<Event::EventManager> m_eventManager;
	std::unique_ptr<Audio::Mixer> m_mixer;

#if !defined(PLATFORM_WEB)
	std::unique_ptr<Platform::Window> m_window;
	std::unique_ptr<Renderer::GraphicsDevice> m_graphicsDevice;
	std::unique_ptr<Renderer::UIRenderer> m_uiRenderer;
	#if defined(EDITOR)
	std::unique_ptr<Renderer::ImGuiRenderer> m_imGuiRenderer;
	#endif
#endif

#ifdef EDITOR
	std::unique_ptr<Debug::Editor> m_editor;
#endif
#ifdef DEBUG
	std::unique_ptr<Debug::Profiler> m_profiler;
#endif
};
}  // namespace Struktur
