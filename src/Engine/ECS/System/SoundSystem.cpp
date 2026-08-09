#include "SoundSystem.h"

#include "Engine/GameContext.h"

// SDL3_mixer mixes on its own thread, unlike raylib's raudio which needed a per-frame UpdateMusicStream() pump - nothing left for this system to do.
void Struktur::System::SoundSystem::Update(GameContext&)
{
}
