#include "GameContext.h"

#include <cstdlib>

#include "Engine/Core/CommandLine.h"

namespace Struktur
{
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

	m_eventManager        = std::make_unique<Event::EventManager>();
	m_gameData            = std::make_unique<Core::GameData>();
	m_graphicsDevice      = std::make_unique<Renderer::GraphicsDevice>();
	m_input               = std::make_unique<Input::Input>(0);
	m_localizationManager = std::make_unique<Localization::LocalizationManager>();
	m_mixer               = std::make_unique<Audio::Mixer>();
#ifdef DEBUG
	m_profiler = std::make_unique<Debug::Profiler>();
#endif
	m_resourceManager = std::make_unique<Resource::ResourceManager>();
	m_timeSystem      = std::make_unique<Core::TimeSystem>();
	m_window          = std::make_unique<Platform::Window>();
}

GameContext::~GameContext() = default;

void GameContext::StopJobs()
{
	m_frameScheduler.reset();
	m_jobSystem->Shutdown();
}

void GameContext::ShutdownCore()
{
	// Every member below (other than the graphics device itself) must be torn down before
	// m_graphicsDevice->Shutdown() runs - bgfx::destroy() is invalid once bgfx has shut down.
	m_eventManager.reset();
	m_gameData.reset();
	m_input.reset();
	m_localizationManager.reset();
	m_mixer.reset();
#ifdef DEBUG
	m_profiler.reset();
#endif
	m_resourceManager.reset();
	m_timeSystem.reset();

	m_graphicsDevice->Shutdown();
	m_graphicsDevice.reset();
	m_window.reset();
	// Last: objects torn down earlier (a game's own services) may still wait on their - by now complete - tasks.
	m_jobSystem.reset();
}

Event::EventManager& GameContext::GetEventManager() const
{
	ASSERT_MSG(m_eventManager.get(), "Event Manager not initialised");
	return *m_eventManager;
}

Core::GameData& GameContext::GetGameData() const
{
	ASSERT_MSG(m_gameData.get(), "GameData not initialised");
	return *m_gameData;
}

Renderer::GraphicsDevice& GameContext::GetGraphicsDevice() const
{
	ASSERT_MSG(m_graphicsDevice.get(), "GraphicsDevice not initialised");
	return *m_graphicsDevice;
}

Input::Input& GameContext::GetInput() const
{
	ASSERT_MSG(m_input.get(), "Input not initialised");
	return *m_input;
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

#ifdef DEBUG
Debug::Profiler& GameContext::GetProfiler() const
{
	ASSERT_MSG(m_profiler.get(), "Profiler not initialised");
	return *m_profiler;
}
#endif

Resource::ResourceManager& GameContext::GetResourceManager() const
{
	ASSERT_MSG(m_resourceManager.get(), "Resource Manager not initialised");
	return *m_resourceManager;
}

Core::TimeSystem& GameContext::GetTimeSystem() const
{
	ASSERT_MSG(m_timeSystem.get(), "TimeSystem not initialised");
	return *m_timeSystem;
}

Platform::Window& GameContext::GetWindow() const
{
	ASSERT_MSG(m_window.get(), "Window not initialised");
	return *m_window;
}
}  // namespace Struktur
