#include "SpriteRenderSystem.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "raylib.h"
#include "raymath.h"

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Shader.h"

#include "Debug/Assertions.h"

void Struktur::System::SpriteRenderSystem::Update(GameContext &context)
{
    entt::registry& registry = context.GetRegistry();
    GameResource::Camera& camera = context.GetCamera();

    ::BeginMode2D(camera.GetRaylibCamera());
    //std::vector<spriteDraw> lateSprites;
    {
        auto view = registry.view<Component::Sprite, Component::WorldTransform>();
        m_spritesToRender.clear();
        m_spritesToRender.reserve(view.size_hint());
        for (auto [entity, sprite, worldTransform] : view.each())
        {
            Core::Resource::TextureResource* texture = sprite.texture.Get();
            if (!texture) continue;

            if (!texture->IsGpuReady())
            {
                texture->LoadToGpu();
            }

            // Add to render list
            m_spritesToRender.push_back({
                entity,
                &sprite,
                &worldTransform,
                sprite.renderPriority  // Assuming this is the new attribute you added
            });
        }

        // Sort by render priority (lower values render first, higher values render on top)
        std::sort(m_spritesToRender.begin(), m_spritesToRender.end(),
            [](const SpriteRenderData& a, const SpriteRenderData& b) {
                return a.renderPriority < b.renderPriority;
            });

        // Render sprites in priority order
        for (const auto& renderData : m_spritesToRender)
        {
            const Component::Sprite& sprite = *renderData.sprite;
            const Component::WorldTransform& worldTransform = *renderData.worldTransform;

            
            Core::Resource::TextureResource* texture = sprite.texture.Get();
            // texture is guaranteed to exist and be GPU ready from previous check
            
            int imageWidth = texture->GetWidth();
            int imageHeight = texture->GetHeight();
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
            
            // This stops a little of the next sprite in the sprite sheet from showing due to rounding error in the GPU
            sourceRec.x += 0.0001f;
            sourceRec.y += 0.0001f;
            sourceRec.width -= 0.0002f;
            sourceRec.height -= 0.0002f;
            
            ::Rectangle destRec{ ::round(worldTransform.position.x * 2) / 2, ::round(worldTransform.position.y * 2) / 2, size.x * worldTransform.scale.x, size.y * worldTransform.scale.x };
            ::Vector2 offset{ sprite.offset.x, sprite.offset.y };
            
            Component::Shader* shader = registry.try_get<Component::Shader>(renderData.entity);

            if (shader)
            {
                Core::GameData& gameDate = context.GetGameData();
                // Set shader uniforms
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "time"), &gameDate.gameTime, SHADER_UNIFORM_FLOAT);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "soulColor"), &shader->color, SHADER_UNIFORM_VEC3);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "glowIntensity"), &shader->glowIntensity, SHADER_UNIFORM_FLOAT);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "rippleSpeed"), &shader->rippleSpeed, SHADER_UNIFORM_FLOAT);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "rippleFrequency"), &shader->rippleFreq, SHADER_UNIFORM_FLOAT);
                
                ::Vector2 resolution = {gameDate.screenWidth, gameDate.screenHeight};
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "resolution"), &resolution, SHADER_UNIFORM_VEC2);

                // Set vertex shader uniforms for wave effect
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "waveAmplitude"), &shader->amplitude, SHADER_UNIFORM_FLOAT);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "waveFrequency"), &shader->frequency, SHADER_UNIFORM_FLOAT);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "waveSpeed"), &shader->speed, SHADER_UNIFORM_FLOAT);
                ::SetShaderValue(shader->shader, ::GetShaderLocation(shader->shader, "waveDirection"), &shader->direction, SHADER_UNIFORM_VEC2);

                ::BeginShaderMode(shader->shader);
            }
            ::DrawTexturePro(texture->texture, sourceRec, destRec, offset, glm::degrees(euler.z), sprite.color);
            if (shader)
            {
                ::EndShaderMode();
            }
        }
    }
    {
        auto view = registry.view<Component::TileMap, Component::WorldTransform>();
        for (auto [entity, tileMap, worldTransform] : view.each())
        {
            Core::Resource::TextureResource* texture = tileMap.texture.Get();
            if (!texture->IsGpuReady())
            {
                texture->LoadToGpu();
            }

            for (auto& gridTile : tileMap.gridTiles)
            {
                ::Rectangle sourceRec{ gridTile.sourcePosition.x, gridTile.sourcePosition.y, (float)tileMap.tileSize, (float)tileMap.tileSize };
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
                // TODO - Move this to a helper function so this line is much more consise
                ::Rectangle DestRec{ gridTile.position.x + ::round(worldTransform.position.x * 2) / 2, gridTile.position.y + ::round(worldTransform.position.y * 2) / 2, (float)tileMap.tileSize, (float)tileMap.tileSize };
                ::DrawTexturePro(texture->texture, sourceRec, DestRec, ::Vector2{ 0,0 }, 0, WHITE);
            }
        }
    }
    ::EndMode2D();
}
