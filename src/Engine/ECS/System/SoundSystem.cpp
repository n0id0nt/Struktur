#include "SoundSystem.h"

#include <raylib.h>

#include "Engine/GameContext.h"
#include "Engine/Resource/MusicResource.h"
#include "Debug/Profiling/Profiler.h"

void Struktur::System::SoundSystem::Update(GameContext &context)
{
    PROFILE_FUNCTION();
    Resource::ResourceManager& resourceManager = context.GetResourceManager();
    const Resource::MusicPool& musicPool = resourceManager.GetMusicPool();
    for (auto [name, music] : musicPool)
    {
        if (music->isLoaded && ::IsMusicStreamPlaying(music->music))
        {
            ::UpdateMusicStream(music->music);
        }
    }
}