#pragma once

#include "raylib.h"
#include "box2d/box2d.h"

#include "Engine/ECS/SystemManager.h"
#include "Debug/Box2DDebugRenderer.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class DebugSystem : public ISystem
        {
        public:
			DebugSystem();

            void Update(GameContext& context) override;

		private:
			Debug::Box2DDebugRenderer m_box2dRenderer;
        };
    }
}
