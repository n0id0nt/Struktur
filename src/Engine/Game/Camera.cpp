#include "Camera.h"

glm::vec2 Struktur::GameResource::Camera::WorldPosToScreenPos(const glm::vec2& worldPos)
{
    ::Vector2 screenPos = GetWorldToScreen2D(::Vector2{ worldPos.x, worldPos.y }, GetRaylibCamera());
    return glm::vec2{ screenPos.x, screenPos.y };
}

glm::vec2 Struktur::GameResource::Camera::ScreenPosToWorldPos(const glm::vec2& screenPos)
{
    ::Vector2 worldPos = GetScreenToWorld2D(::Vector2{ screenPos.x, screenPos.y }, GetRaylibCamera());
    return glm::vec2{ worldPos.x, worldPos.y };
}

::Camera2D Struktur::GameResource::Camera::GetRaylibCamera()
{
    return ::Camera2D{
        ::Vector2{ offset.x, offset.y },
        ::Vector2{ target.x, target.y },
        rotation,
        zoom,
    };
}
