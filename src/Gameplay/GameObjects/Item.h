#pragma once

#include <format>
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

#include "Engine/Core/Resource/TextureResource.h"

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

namespace Struktur
{
    namespace Item
    {
        void Create(GameContext& context, entt::entity entity, const std::string& name, bool canBeReturned)
        {
            entt::registry& registry = context.GetRegistry();
            System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
            Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
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

                Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture = resourceManager.GetTexture(std::format("assets/Tiles/Items/{}.png", interactionId).c_str());
                registry.emplace<Component::Sprite>(entity, texture, color, glm::vec2(32, 48), 1, 1, false, 0, 1);
            }

            registry.emplace<Component::Interactable>(entity, name, canBeReturned);
        }
    }
}