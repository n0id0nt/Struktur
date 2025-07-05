#pragma once

#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace Struktur
{
	namespace Core
	{
        class GameContext;

        class ISystem
        {
        public:
            //ISystem() = default;
            //ISystem(const ISystem&) = delete;
            //ISystem& operator=(const ISystem&) = delete;

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
                static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");

                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = system.get();
                m_updateSystems.push_back(std::move(system));

                std::type_index typeIndex = std::type_index(typeid(T));
                systemCache[typeIndex] = ptr;

                return *ptr;
            }
            template<typename T, typename... Args>
            T& AddRenderSystem(Args&&... args)
            {
                static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");

                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = system.get();
                m_renderSystems.push_back(std::move(system));

                std::type_index typeIndex = std::type_index(typeid(T));
                systemCache[typeIndex] = ptr;

                return *ptr;
            }

            template<typename T>
            T& GetSystem()
            {
                std::type_index typeIndex = std::type_index(typeid(T));
        
                auto it = systemCache.find(typeIndex);
                if (it != systemCache.end()) {
                    return static_cast<T*>(it->second);
                }
                
                return *system;
            }

        private:
            std::vector<std::unique_ptr<ISystem>> m_updateSystems;
            std::vector<std::unique_ptr<ISystem>> m_renderSystems;

            std::unordered_map<std::type_index, ISystem*> systemCache;
        };
    }
}