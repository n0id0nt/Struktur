#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace Component
{
struct Sprite;
struct Transform;
}  // namespace Component

namespace System
{
class SpriteRenderSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Sprite Render System";
	}
};
}  // namespace System
}  // namespace Struktur
