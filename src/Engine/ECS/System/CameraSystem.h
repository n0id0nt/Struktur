#pragma once
#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{

    class GameContext;

	namespace GameResource
	{
		struct Camera;
	}
	namespace Component
	{
		struct Camera;
	}
	namespace System
	{
		class CameraSystem : public ISystem
		{
		public:
			void Update(GameContext& context) override;

			glm::vec2 CalculateSmoothedPosition(float gameTime, float deltaTime, int screenWidth, int screenHeight, Component::Camera* cameraComponent, const glm::vec2& cameraComponentPos, GameResource::Camera& camera);
			glm::vec2 TargetPosition(float gameTime, float deltaTime, int screenWidth, int screenHeight, Component::Camera* cameraComponent, const glm::vec2& cameraComponentPos, GameResource::Camera& camera);
			void CalculateCameraShake(float gameTime, float deltaTime, int screenWidth, int screenHeight, Component::Camera* cameraComponent, GameResource::Camera& camera);
		};
	}
}