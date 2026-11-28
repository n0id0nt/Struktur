#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Moves the world camera: the camera entity follows the rider along the road (World::FollowRoad), and the
// result becomes the context's camera, which the world renderer draws through.
class CameraSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "Camera System";
	}
};
}  // namespace System
}  // namespace Struktur
