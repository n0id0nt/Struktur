#include "DebugSystem.h"

#include "raylib.h"

#include "Engine/GameContext.h"

void Struktur::System::DebugSystem::Update(GameContext &context)
{
    ::DrawFPS(10,10);
}
