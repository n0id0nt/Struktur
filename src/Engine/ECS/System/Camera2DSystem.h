#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

namespace Struktur
{

class GameContext;

namespace World
{
struct Camera2D;
}
namespace Component
{
struct Camera;
}
namespace System
{
class Camera2DSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	glm::vec2 CalculateSmoothedPosition(float gameTime, float deltaTime, int screenWidth, int screenHeight,
	                                    Component::Camera* cameraComponent, const glm::vec2& cameraComponentPos,
	                                    World::Camera2D& camera);
	glm::vec2 TargetPosition(float gameTime, float deltaTime, int screenWidth, int screenHeight,
	                         Component::Camera* cameraComponent, const glm::vec2& cameraComponentPos,
	                         World::Camera2D& camera);
	glm::vec2 ConfineToBounds(const glm::vec2& position, int screenWidth, int screenHeight,
	                          Component::Camera* cameraComponent);
	void CalculateCameraShake(float gameTime, float deltaTime, int screenWidth, int screenHeight,
	                          Component::Camera* cameraComponent, World::Camera2D& camera);

	void AddCameraTrauma(GameContext& context, entt::entity entity, float trauma);
	void ResetCameraTrauma(GameContext& context, entt::entity entity);

	std::string Name() const override
	{
		return "Camera System";
	}
};
}  // namespace System
}  // namespace Struktur
