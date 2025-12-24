#pragma once

#include <unordered_map>
#include <wren.hpp>
#include <string>

namespace Struktur
{
	class GameContext;

	namespace Wren
	{
		struct WrenScriptComponent
		{
			std::string module;
			std::string className;
			WrenHandle* classHandle = nullptr;
			WrenHandle* startMethodHandle = nullptr;
			WrenHandle* updateMethodHandle = nullptr;
			WrenHandle* onDestroyMethodHandle = nullptr;
			WrenHandle* onEventMethodHandle = nullptr;
			bool isInitialised = false;
		};

		class WrenScriptComponentRegistry
		{
		private:
#ifdef DEBUG
			// Track which scripts have been loaded
            //TODO make this a debug only feature
            std::unordered_map<std::string, time_t> m_fileModificationTimes;
#endif
			std::unordered_map<std::string, WrenScriptComponent> m_scriptComponents;

		public:
			WrenScriptComponentRegistry() : m_scriptComponents() {}
			~WrenScriptComponentRegistry() {}

			void RegisterScriptComponent(std::string module, std::string className);
			bool LoadAllScriptComponents(GameContext& context);
			WrenScriptComponent* TryGetScriptComponent(std::string className);
			void Shutdown(GameContext& context);

#ifdef DEBUG
		// Hot reload support
		void CheckForScriptChanges(GameContext& context);
		void ReloadScript(GameContext& context);

		private:
            time_t GetFileModificationTime(const std::string& path);
#endif
        };
    }
}