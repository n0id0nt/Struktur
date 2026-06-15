#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace Component
{
struct Sprite;
struct WorldTransform;
}  // namespace Component

namespace System
{
class SpriteRenderSystem : public ISystem
{
   private:
	struct SpriteRenderData
	{
		entt::entity entity;
		Component::Sprite* sprite;
		Component::WorldTransform* worldTransform;
		int renderPriority;
	};

   public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Sprite Render System";
	}

   private:
	std::vector<SpriteRenderData> m_spritesToRender;
};
}  // namespace System
}  // namespace Struktur
