#include "CameraSystem.h"

#include <raymath.h>
#include "glm/gtc/quaternion.hpp."

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/Game/Camera.h"
#include "Engine/Util/Noise.h"

void Struktur::System::CameraSystem::Update(GameContext& context)
{
    entt::registry& registry = context.GetRegistry();
    Core::GameData& gameData = context.GetGameData();

    auto view = registry.view<Struktur::Component::Camera, Struktur::Component::WorldTransform>();

    entt::entity focusedCameraEntity;
    Struktur::Component::Camera* focusedCameraComponent = nullptr;
    Struktur::Component::WorldTransform* focusedTransformComponent = nullptr;
    int highestPriority = INT_MIN;
    for (auto [entity, camera, worldTransform] : view.each())
    {
        if (camera.cameraPriority > highestPriority)
        {
            highestPriority = camera.cameraPriority;
            focusedCameraEntity = entity;
            focusedCameraComponent = &camera;
            focusedTransformComponent = &worldTransform;
        }
    }

    if (focusedCameraComponent && focusedTransformComponent)
    {
        glm::vec3 euler = glm::eulerAngles(focusedTransformComponent->rotation);

        float gameTime = gameData.gameTime;
        float deltaTime = gameData.deltaTime;
        int screenWidth = gameData.screenWidth;
        int screenHeight = gameData.screenHeight;
        GameResource::Camera& out_camera = context.GetCamera();
        //out_camera.target = out_camera.previousCameraPosition;
        glm::vec2 newPos = focusedCameraComponent->forcePosition ? TargetPosition(gameTime, deltaTime, screenWidth, screenHeight, focusedCameraComponent, focusedTransformComponent->position, out_camera)
            : CalculateSmoothedPosition(gameTime, deltaTime, screenWidth, screenHeight, focusedCameraComponent, focusedTransformComponent->position, out_camera);
        focusedCameraComponent->forcePosition = false;
        out_camera.target = newPos;
        out_camera.offset = glm::vec2{ screenWidth/2.f + focusedCameraComponent->offset.x, screenHeight/2.f + focusedCameraComponent->offset.x };
        out_camera.zoom = focusedCameraComponent->zoom;
        out_camera.rotation = focusedCameraComponent->angle;
        out_camera.previousCameraPosition = newPos;
        //out_camera.previousCameraAngle = angle;

        // exclude shake from previous position
        CalculateCameraShake(gameTime, deltaTime, screenWidth, screenHeight, focusedCameraComponent, out_camera);
    }
}

glm::vec2 Struktur::System::CameraSystem::CalculateSmoothedPosition(float gameTime, float deltaTime, int screenWidth, int screenHeight, Struktur::Component::Camera* cameraComponent, const glm::vec2& cameraComponentPos, GameResource::Camera& camera)
{
	glm::vec2 cameraComponentScreenPos = camera.WorldPosToScreenPos(cameraComponentPos);
    glm::vec2 newPos{};
	// x
	{
		if (cameraComponent->offset.x + cameraComponent->deadZone.x < cameraComponentScreenPos.x)
		{
			newPos.x = ::Lerp(screenWidth / 2.f, cameraComponentScreenPos.x - cameraComponent->offset.x - cameraComponent->deadZone.x, cameraComponent->damping.x * deltaTime);
		}
		else if (cameraComponent->offset.x - cameraComponent->deadZone.x > cameraComponentScreenPos.x)
		{
			newPos.x = ::Lerp(screenWidth / 2.f, cameraComponentScreenPos.x - cameraComponent->offset.x + cameraComponent->deadZone.x, cameraComponent->damping.x * deltaTime);
		}
	}
	// y 
	{
		if (cameraComponent->offset.y + cameraComponent->deadZone.y < cameraComponentScreenPos.y)
		{
			newPos.y = ::Lerp(screenHeight / 2.f, cameraComponentScreenPos.y - cameraComponent->offset.y - cameraComponent->deadZone.y, cameraComponent->damping.y * deltaTime);
		}
		else if (cameraComponent->offset.y - cameraComponent->deadZone.y > cameraComponentScreenPos.y)
		{
			newPos.y = ::Lerp(screenHeight / 2.f, cameraComponentScreenPos.y - cameraComponent->offset.y + cameraComponent->deadZone.y, cameraComponent->damping.y * deltaTime);
		}
	}

    return camera.ScreenPosToWorldPos(newPos);
}

glm::vec2 Struktur::System::CameraSystem::TargetPosition(float gameTime, float deltaTime, int screenWidth, int screenHeight, Struktur::Component::Camera* cameraComponent, const glm::vec2& cameraComponentPos, GameResource::Camera& camera)
{
    return cameraComponentPos;
}

void Struktur::System::CameraSystem::CalculateCameraShake(float gameTime, float deltaTime, int screenWidth, int screenHeight, Struktur::Component::Camera* cameraComponent, GameResource::Camera& camera)
{
    float trauma = cameraComponent->trauma;
    if (trauma > 0.f)
    {
        const int seed = 0;
        float shake = std::pow(trauma, 2.f);
        float angle = cameraComponent->maxAngle * shake * Util::Noise::PerlinNoise1(seed, gameTime / cameraComponent->shakeAmplitude);
        float xOffset = cameraComponent->maxOffset * shake * Util::Noise::PerlinNoise1(seed + 1, gameTime / cameraComponent->shakeAmplitude);
        float yOffset = cameraComponent->maxOffset * shake * Util::Noise::PerlinNoise1(seed + 2, gameTime / cameraComponent->shakeAmplitude);

        cameraComponent->trauma = trauma - deltaTime / cameraComponent->traumaTime;
        camera.target = glm::vec2{ camera.target.x + xOffset, camera.target.y + yOffset };
        //camera.previousCameraAngle = camera.rotation + angle;
        camera.rotation = camera.rotation + angle;
    }
    else
    {
        cameraComponent->trauma = 0;
    }
}

void Struktur::System::CameraSystem::AddCameraTrauma(GameContext &context, entt::entity entity, float trauma)
{
    entt::registry& registry = context.GetRegistry();
    Struktur::Component::Camera* cameraComponent = registry.try_get<Struktur::Component::Camera>(entity);
    if (!cameraComponent)
    {
        BREAK_MSG("Entity provided does not contain a camera component");
        return;
    }
    cameraComponent->trauma += trauma;
	if (cameraComponent->trauma > 1.0f) cameraComponent->trauma = 1.0f;
}

void Struktur::System::CameraSystem::ResetCameraTrauma(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    Struktur::Component::Camera* cameraComponent = registry.try_get<Struktur::Component::Camera>(entity);
    if (!cameraComponent)
    {
        BREAK_MSG("Entity provided does not contain a camera component");
        return;
    }
    cameraComponent->trauma = 0.0f;
}
