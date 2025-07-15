#pragma once

#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "Debug/Assertions.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class ISystem
        {
        public:
            virtual void Update(GameContext& context) = 0;
        };

        class SystemManager
        {
        public:
            SystemManager() {}
            ~SystemManager() {}

            void Update(GameContext& context);

            template<typename T, typename... Args>
            T& AddUpdateSystem(Args&&... args)
            {
                static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
                std::type_index typeIndex = std::type_index(typeid(T));

                m_updateSystems.push_back(typeIndex);
                
                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = system.get();
                m_systemMap[typeIndex] = std::move(system);

                return *ptr;
            }

            template<typename T, typename... Args>
            T& AddRenderSystem(Args&&... args)
            {
                static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
                std::type_index typeIndex = std::type_index(typeid(T));

                m_renderSystems.push_back(typeIndex);

                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = system.get();
                m_systemMap[typeIndex] = std::move(system);

                return *ptr;
            }

            template<typename T>
            T* TryGetSystem()
            {
                std::type_index typeIndex = std::type_index(typeid(T));
        
                auto it = m_systemMap.find(typeIndex);
                if (it != m_systemMap.end()) {
                    return static_cast<T*>(it->second.get());
                }
                
                return nullptr;
            }

            template<typename T>
            T& GetSystem()
            {
                T* system = TryGetSystem<T>();
                ASSERT_MSG(system, "System Type Not Registered");
                return *system;
            }

        private:
            std::vector<std::type_index> m_updateSystems;
            std::vector<std::type_index> m_renderSystems;

            std::unordered_map<std::type_index, std::unique_ptr<ISystem>> m_systemMap;
        };
    }
}
