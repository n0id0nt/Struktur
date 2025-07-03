#pragma once

#include <vector>
#include <memory>

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
            void Render(GameContext& context);

            template<typename T, typename... Args>
            T& AddUpdateSystem(Args&&... args)
            {
                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = system.get();
                m_updateSystems.push_back(std::move(system));
                return *ptr;
            }
            template<typename T, typename... Args>
            T& AddRenderSystem(Args&&... args)
            {
                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = system.get();
                m_renderSystems.push_back(std::move(system));
                return *ptr;
            }

        private:
            std::vector<std::unique_ptr<ISystem>> m_updateSystems;
            std::vector<std::unique_ptr<ISystem>> m_renderSystems;
        };
    }
}