#pragma once

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/Shader.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/ShaderSystem.h"

#include "Engine/Resource/TextureResource.h"

namespace Struktur
{
	namespace NPC
	{
		struct NPCData
		{
			int spriteIndex;
			int xOffset, yOffset;
		};

		static std::unordered_map<std::string, NPCData> s_spriteDataMap = {
			{"Scholar", { 0, 48, 64 }, },
			{"Gardener", { 6, 48, 64 }, },
			{"Cook", { 4, 48, 64 }, },
			{"Inventor", { 2, 48, 64 }, },
			{"Dreamer", { 5, 60, 48 }, },
			{"Astronomer", { 8, 48, 64 }, },
			{"Merchant", { 1, 48, 64 }, },
			{"Guardian", { 7, 48, 64 }, },
			{"Cordelia", { 3, 48, 64 }, },
		};

		void Create(GameContext& context, entt::entity entity, const std::string& name)
		{
			entt::registry& registry = context.GetRegistry();
			System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
			Resource::ResourceManager& resourceManager = context.GetResourceManager();
			System::SystemManager& systemManager = context.GetSystemManager();
			auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
			auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();
			auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();
			Resource::ResourcePtr<Resource::TextureResource> texture = resourceManager.GetTexture("assets/Tiles/NPCs.png");

			const NPCData& npcData = s_spriteDataMap[name];
			auto& worldTransform = registry.get<Component::WorldTransform>(entity);
			registry.emplace<Component::Sprite>(entity, texture, WHITE, glm::vec2(npcData.xOffset, npcData.yOffset), 9, 1, false, npcData.spriteIndex, (int)worldTransform.position.y);
			b2BodyDef kinematicBodyDef;
			kinematicBodyDef.type = b2_staticBody;
			b2CircleShape playerShape;
			playerShape.m_radius = 0.25f;
			physicsSystem.CreatePhysicsBody(context, entity, kinematicBodyDef, playerShape);
			Component::PhysicsBody& physicsBody = registry.get<Component::PhysicsBody>(entity);
			physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
			physicsBody.syncToPhysics = true;    // Let transform drive physics

			//::Shader soulShader = ::LoadShader(0, "assets/Shaders/SoulEffect_100.fs");
			//bool useShader = (soulShader.id > 0);
			//if (!useShader)
			//{
			//    DEBUG_WARNING("Failed to load soul shader");
			//}
			//registry.emplace<Component::Shader>(entity, soulShader);
			//
			//System::ShaderSystem& shaderSystem = context.GetSystemManager().GetSystem<System::ShaderSystem>();
			//shaderSystem.SetUniform(context, entity, "soulColor", glm::vec3{0.3f, 0.7f, 1.0f});
			//shaderSystem.SetUniform(context, entity, "glowIntensity", 0.05f);
			//shaderSystem.SetUniform(context, entity, "rippleSpeed", 1.0f);
			//shaderSystem.SetUniform(context, entity, "rippleFrequency", 15.0f);
			//shaderSystem.SetUniform(context, entity, "waveAmplitude", 0.05f);
			//shaderSystem.SetUniform(context, entity, "waveFrequency", 8.0f);
			//shaderSystem.SetUniform(context, entity, "waveSpeed", 3.0f);
			//shaderSystem.SetUniform(context, entity, "waveDirection", glm::vec2{1.0f, 0.3f});
			//shaderSystem.SetUniform(context, entity, "scanlineIntensity", 0.1f);
			//shaderSystem.SetUniform(context, entity, "chromaticAberration", 0.005f);
			//shaderSystem.SetUniform(context, entity, "glitchFrequency", 0.05f);
			//shaderSystem.SetUniform(context, entity, "holographicShift", 10.0f);
		}
	}
}