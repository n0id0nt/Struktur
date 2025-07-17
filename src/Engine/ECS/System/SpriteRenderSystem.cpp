#include "SpriteRenderSystem.h"

#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "raylib.h"
#include "raymath.h"

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"

void Struktur::System::SpriteRenderSystem::Update(GameContext &context)
{
    Core::ResourcePool& resourcePool = context.GetResourcePool();
    entt::registry& registry = context.GetRegistry();
    GameResource::Camera& camera = context.GetCamera();

    ::BeginMode2D(camera.GetRaylibCamera());
    //std::vector<spriteDraw> lateSprites;
    {
        auto view = registry.view<Component::Sprite, Component::WorldTransform>();
        for (auto [entity, sprite, worldTransform] : view.each())
        {
            const Image& image = resourcePool.RetrieveImage(sprite.fileName);
            int imageWidth = image.width;
            int imageHeight = image.height;
            glm::vec3 euler = glm::eulerAngles(worldTransform.rotation);

            int index = sprite.index;
            ASSERT_MSG(sprite.columns > 0, "Sprite needs to have at least one column");
            ASSERT_MSG(sprite.rows > 0, "Sprite needs to have at least one row");
            glm::vec2 size = glm::vec2(imageWidth / sprite.columns, imageHeight / sprite.rows);
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

            ::Rectangle destRec{ ::round(worldTransform.position.x * 2) / 2, ::round(worldTransform.position.y * 2) / 2, size.x * worldTransform.scale.x, size.y * worldTransform.scale.x };

            ::Vector2 offset{ sprite.offset.x, sprite.offset.y };
            
            ::Texture2D texture = resourcePool.RetrieveTexture(sprite.fileName);
            ::DrawTexturePro(texture, sourceRec, destRec, offset, glm::degrees(euler.z), sprite.color);
        }
    }
    {
        auto view = registry.view<Component::TileMap, Component::WorldTransform>();
        for (auto [entity, tileMap, worldTransform] : view.each())
        {
            ::Texture2D texture = resourcePool.RetrieveTexture(tileMap.imagePath);

            for (auto& gridTile : tileMap.gridTiles)
            {
                ::Rectangle sourceRec{ gridTile.sourcePosition.x, gridTile.sourcePosition.y, tileMap.tileSize, tileMap.tileSize };
                switch (gridTile.flipBit)
                {
                case GameResource::TileMap::FlipBit::BOTH:
                    sourceRec.width *= -1;
                    sourceRec.height *= -1;
                    break;
                case GameResource::TileMap::FlipBit::HORIZONTAL:
                    sourceRec.width *= -1;
                    break;
                case GameResource::TileMap::FlipBit::VERTIAL:
                    sourceRec.height *= -1;
                    break;
                }
                // this stops you from seeing a little bit of the neighbouring sprite
                sourceRec.x += 0.0001f;
                sourceRec.y += 0.0001f;
                sourceRec.width -= 0.0002f;
                sourceRec.height -= 0.0002f;
                ::Rectangle DestRec{ gridTile.position.x, gridTile.position.y, tileMap.tileSize, tileMap.tileSize };
                ::DrawTexturePro(texture, sourceRec, DestRec, ::Vector2{ 0,0 }, 0, WHITE);
            }
        }
    }
    ::EndMode2D();
}
