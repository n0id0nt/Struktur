#pragma once
#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

#include "raylib.h"
#include "raymath.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/Player.h"
#include "ECS/Component/Sprite.h"
#include "Engine/Core/GameContext.h"

namespace Struktur
{
	namespace System
	{
        class SpriteRenderSystem : public Core::ISystem
        {
        public:
            void Update(Core::GameContext& context) override
            {
                Struktur::Core::ResourcePool& resourcePool = context.GetResourcePool();
                entt::registry& registry = context.GetRegistry();
                auto view = registry.view<Component::Sprite, Component::WorldTransform>();
                for (auto [entity, sprite, worldTransform] : view.each())
                {
                    glm::vec3 scaleVec;
                    glm::quat rotationQuat{};
                    glm::vec3 translationVec;
                    glm::vec3 skewVec;
                    glm::vec4 perspectiveVec;
                    glm::decompose(worldTransform.matrix, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);
                    glm::vec3 euler = glm::eulerAngles(rotationQuat);
                    
                    const Image& image = resourcePool.RetrieveImage(sprite.fileName);
                    int imageWidth = image.width;
                    int imageHeight = image.height;

                    int index = sprite.index;
                    ASSERT_MSG(sprite.columns > 0, "Sprite needs to have at least one column");
                    ASSERT_MSG(sprite.rows > 0, "Sprite needs to have at least one row");
                    glm::vec2 size = glm::vec2(imageWidth/sprite.columns, imageHeight/sprite.rows);
                    int x = (index % sprite.columns) * size.x;
                    int y = std::floor(index / sprite.columns) * size.y;

                    ::Rectangle sourceRec{ (float)x, (float)y, size.x, size.y };

                    if (sprite.flipped)
                    {
                        sourceRec.width *= -1;
                    }

                    // this stops a little of the next sprite in the sprite sheet from showing due to rounding error in the GPU
                    sourceRec.x += 0.0001f;
                    sourceRec.y += 0.0001f;
                    sourceRec.width -= 0.0002f;
                    sourceRec.height -= 0.0002f;

                    ::Rectangle destRec{ ::round(translationVec.x * 2) / 2, ::round(translationVec.y * 2) / 2, size.x * scaleVec.x, size.y * scaleVec.x };

                    ::Vector2 offset{ sprite.offset.x, sprite.offset.y };
            
                    ::DrawTexturePro(resourcePool.RetrieveTexture(sprite.fileName), sourceRec, destRec, offset, glm::degrees(euler.z), sprite.color);
                }
            }
        };
    }
}