#include "AnimationSystem.h"

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/Sprite.h"

void Struktur::System::AnimationSystem::Update(GameContext& context)
{
    entt::registry& registry = context.GetRegistry();
    Core::GameData& gameData = context.GetGameData();
    double gameTime = gameData.gameTime;

    auto view = registry.view<Component::Sprite, Component::SpriteAnimation>();
	for (auto [entity, sprite, spriteAnimation] : view.each())
	{
		// get the current animation frame
		float animationTime = gameTime - spriteAnimation.animationStartTime;
		Struktur::Animation::SpriteAnimation curAnimation = spriteAnimation.animations[spriteAnimation.curAnimation];
		if (curAnimation.loop) 
		{
			animationTime = fmod(animationTime, curAnimation.animationTime);
		}
		else if (animationTime > curAnimation.animationTime)
		{
			animationTime = curAnimation.animationTime;
		}

		int frame = curAnimation.startFrame + (int)std::floor((curAnimation.endFrame - curAnimation.startFrame) * animationTime / curAnimation.animationTime);
		sprite.index = frame;
	}
}

void Struktur::System::AnimationSystem::AddAnimation(GameContext& context, entt::entity entity, const std::string& animationName, const Animation::SpriteAnimation& animation)
{
    entt::registry& registry = context.GetRegistry();
	auto& animationComponent = registry.get<Component::SpriteAnimation>(entity);

	auto it = animationComponent.animations.find(animationName);
	if (it == animationComponent.animations.end())
	{
		animationComponent.animations[animationName] = animation;
	}
	else
	{
		BREAK_MSG("animation already exists"); 
	}
}

void Struktur::System::AnimationSystem::PlayAnimation(GameContext& context, entt::entity entity, const std::string& animationName)
{
    entt::registry& registry = context.GetRegistry();
	auto& animationComponent = registry.get<Component::SpriteAnimation>(entity);
    
    Core::GameData& gameData = context.GetGameData();
    double gameTime = gameData.gameTime;

	animationComponent.curAnimation = animationName;
	animationComponent.animationStartTime = gameTime;
}

bool Struktur::System::AnimationSystem::IsAnimationPlaying(GameContext& context, entt::entity entity, const std::string& animationName)
{
    entt::registry& registry = context.GetRegistry();
    auto& animationComponent = registry.get<Component::SpriteAnimation>(entity);
    
	Struktur::Animation::SpriteAnimation curAnimation = animationComponent.animations[animationComponent.curAnimation];
    if (curAnimation.loop) 
    {
        return true;
    }

    Core::GameData& gameData = context.GetGameData();
    double gameTime = gameData.gameTime;

	float animationTime = gameTime - animationComponent.animationStartTime;
	return animationTime <= curAnimation.animationTime;
}