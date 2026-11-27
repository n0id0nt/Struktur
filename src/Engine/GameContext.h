#pragma once

#include <memory>
#include <string>

#include "Debug/Assertions.h"
#include "Engine/Audio/Mixer.h"
#include "Engine/Core/GameData.h"
#include "Engine/Core/TimeSystem.h"
#include "Engine/Event/EventManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Jobs/FrameScheduler.h"
#include "Engine/Jobs/JobSystem.h"
#include "Engine/Localization/LocalizationManager.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Resource/ResourceManager.h"
#ifdef DEBUG
	#include "Debug/Profiling/Profiler.h"
#endif
#ifdef STRUKTUR_ECS
	#include "entt/fwd.hpp"
#endif

namespace Struktur
{
// The libraries' service types. Only declared here, so core doesn't depend on them - include a service's own header to
// use it.
#ifdef STRUKTUR_ECS
namespace System
{
class GameObjectManager;
class SystemManager;
}  // namespace System
#endif
#ifdef STRUKTUR_WREN
namespace Wren
{
class WrenScriptComponentRegistry;
class WrenScriptEngine;
class WrenStateManager;
}  // namespace Wren
#endif
#ifdef STRUKTUR_UI
namespace UI
{
class InputGlyphs;
class UIManager;
}  // namespace UI
namespace Renderer
{
class UIRenderer;
}
#endif
#ifdef STRUKTUR_PHYSICS
namespace Physics
{
class CollisionLayers;
class PhysicsWorld;
}  // namespace Physics
#endif
#ifdef STRUKTUR_DIALOGUE
namespace Dialogue
{
class DialogueManager;
class DialogueRegistry;
class VariableSubstitutionSystem;
}  // namespace Dialogue
#endif
#ifdef STRUKTUR_FLAG
namespace Flag
{
class FlagManager;
}
#endif
#ifdef STRUKTUR_FITNESS
namespace Fitness
{
class FitnessManager;
}
#endif
#ifdef EDITOR
namespace Debug
{
class Editor;
}
namespace Renderer
{
class ImGuiRenderer;
}
#endif

// What every game has: the core services, held here, and the getters the libraries need, which each game's own context
// implements beside whatever it adds. A library's getters exist only in builds that use it (STRUKTUR_<NAME>).
class GameContext
{
public:
	GameContext();
	virtual ~GameContext();

	GameContext(const GameContext&)            = delete;
	GameContext& operator=(const GameContext&) = delete;

	// Destroys every service, in order. Call once, after the game loop has ended.
	virtual void Shutdown() = 0;

	// --- core ----------------------------------------------------------------------------------------------------
	Event::EventManager& GetEventManager() const;
	Core::GameData& GetGameData() const;
	Renderer::GraphicsDevice& GetGraphicsDevice() const;
	Input::Input& GetInput() const;
	Jobs::JobSystem& GetJobSystem() const;
	Jobs::FrameScheduler& GetFrameScheduler() const;
	Localization::LocalizationManager& GetLocalizationManager() const;
	Audio::Mixer& GetMixer() const;
#ifdef DEBUG
	Debug::Profiler& GetProfiler() const;
#endif
	Resource::ResourceManager& GetResourceManager() const;
	Core::TimeSystem& GetTimeSystem() const;
	Platform::Window& GetWindow() const;

	// --- libraries -----------------------------------------------------------------------------------------------
#ifdef STRUKTUR_ECS
	virtual entt::registry& GetRegistry() const                     = 0;
	virtual System::GameObjectManager& GetGameObjectManager() const = 0;
	virtual System::SystemManager& GetSystemManager() const         = 0;
	#ifdef STRUKTUR_WREN
	virtual Wren::WrenScriptComponentRegistry& GetWrenScriptComponentRegistry() const = 0;
	#endif
#endif
#ifdef STRUKTUR_WREN
	virtual Wren::WrenScriptEngine& GetWrenScriptEngine() const = 0;
	virtual Wren::WrenStateManager& GetWrenStateManager() const = 0;
#endif
#ifdef STRUKTUR_UI
	virtual UI::InputGlyphs& GetInputGlyphs() const     = 0;
	virtual UI::UIManager& GetUIManager() const         = 0;
	virtual Renderer::UIRenderer& GetUIRenderer() const = 0;
#endif
#ifdef STRUKTUR_PHYSICS
	virtual Physics::CollisionLayers& GetCollisionLayers() const = 0;
	virtual Physics::PhysicsWorld& GetPhysicsWorld() const       = 0;
#endif
#ifdef STRUKTUR_DIALOGUE
	virtual Dialogue::DialogueManager& GetDialogueManager() const                       = 0;
	virtual Dialogue::DialogueRegistry& GetDialogueRegistry() const                     = 0;
	virtual Dialogue::VariableSubstitutionSystem& GetVariableSubstitutionSystem() const = 0;
#endif
#ifdef STRUKTUR_FLAG
	virtual Flag::FlagManager& GetFlagManager() const = 0;
#endif
#ifdef STRUKTUR_FITNESS
	virtual Fitness::FitnessManager& GetFitnessManager() const = 0;
#endif
#ifdef EDITOR
	virtual Debug::Editor& GetEditor() const                  = 0;
	virtual Renderer::ImGuiRenderer& GetImGuiRenderer() const = 0;
#endif

protected:
	// First thing in a game's Shutdown: running jobs finish, queued ones and their main-thread follow-ups are dropped,
	// so nothing is torn down while a worker might still be using it.
	void StopJobs();
	// Last thing in a game's Shutdown, once everything that draws or loads is gone: the core services, ending with the
	// graphics device, the window and the job system.
	void ShutdownCore();

private:
	std::unique_ptr<Event::EventManager> m_eventManager;
	std::unique_ptr<Core::GameData> m_gameData;
	std::unique_ptr<Renderer::GraphicsDevice> m_graphicsDevice;
	std::unique_ptr<Input::Input> m_input;
	std::unique_ptr<Jobs::JobSystem> m_jobSystem;
	std::unique_ptr<Jobs::FrameScheduler> m_frameScheduler;
	std::unique_ptr<Localization::LocalizationManager> m_localizationManager;
	std::unique_ptr<Audio::Mixer> m_mixer;
#ifdef DEBUG
	std::unique_ptr<Debug::Profiler> m_profiler;
#endif
	std::unique_ptr<Resource::ResourceManager> m_resourceManager;
	std::unique_ptr<Core::TimeSystem> m_timeSystem;
	std::unique_ptr<Platform::Window> m_window;
};
}  // namespace Struktur
