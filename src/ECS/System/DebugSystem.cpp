#include "DebugSystem.h"

#include "raylib.h"

#include "Engine/Core/GameContext.h"

void Struktur::System::DebugSystem::Update(Core::GameContext &context)
{
    ::DrawFPS(10,10);
}
