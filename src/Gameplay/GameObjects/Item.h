#pragma once

#include <Vector>

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

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"

#include "Engine/Resource/TextureResource.h"


namespace Struktur
{
    namespace Item
    {
        static const std::vector<std::string> s_itemsWithNoImage = {
            "Yellow Pedestal Inactive",
            "Blue Pedestal Inactive",
            "Red Pedestal Inactive",
            "Green Pedestal Inactive",
            "Yellow Pedestal Active",
            "Blue Pedestal Active",
            "Red Pedestal Active",
            "Green Pedestal Active",
            "Safe",
            "Memory Palace",
        };

        struct ItemData
        {
            int spriteIndex;
            //int xOffset, yOffset;
        };
        static std::unordered_map<std::string, ItemData> s_spriteDataMap = {
            {"Love Letter", { 10, }, },
            {"Hammer", { 9, }, },
            {"Star Chart", { 15, }, },
            {"Ornate Key", { 11, }, },
            {"Ancient Tome", { 1, }, },
            {"Rose", { 14, }, },
            {"Fresh Bread", { 2, }, },
            {"Tool Box", { 16, }, },
            {"Personal Diary", { 12, }, },
            {"Telescope", { 4, }, },
            {"Gold Coins", { 3, }, },
            {"Ancient Seal", { 0, }, },
            {"Red Pedestal", { 6, }, },
            {"Green Pedestal", { 7, }, },
            {"Blue Pedestal", { 8, }, },
            {"Yellow Pedestal", { 9, }, },
        };

        void Create(GameContext& context, entt::entity entity, const std::string& name, bool canBeReturned)
        {
            entt::registry& registry = context.GetRegistry();
            System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
            Resource::ResourceManager& resourceManager = context.GetResourceManager();
            System::SystemManager& systemManager = context.GetSystemManager();
            auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
            
            if (std::find(s_itemsWithNoImage.begin(), s_itemsWithNoImage.end(), name) == s_itemsWithNoImage.end())
            {
                std::string interactionId = name;
                ::Color color = WHITE;
                const std::string suffix = " Return";
                if (interactionId.length() >= suffix.length() && 
                    interactionId.substr(interactionId.length() - suffix.length()) == suffix)
                {
                    color = ::Color{0,0,0,100};
                    // Remove the suffix
                    interactionId = interactionId.substr(0, interactionId.length() - suffix.length());
                }

                auto& itemData = s_spriteDataMap[interactionId];
                Resource::ResourcePtr<Resource::TextureResource> texture = resourceManager.GetTexture("assets/Tiles/Items.png");
                registry.emplace<Component::Sprite>(entity, texture, color, glm::vec2(32, 48), 9, 2, false, itemData.spriteIndex, 1);
            }

            registry.emplace<Component::Interactable>(entity, name, canBeReturned);
        }
    }
}