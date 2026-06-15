#include "SoundSystem.h"

#include <raylib.h>

#include "Engine/GameContext.h"
#include "Engine/Resource/MusicResource.h"

void Struktur::System::SoundSystem::Update(GameContext& context)
{
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	const Resource::MusicPool& musicPool       = resourceManager.GetMusicPool();
	for (auto [name, music] : musicPool)
	{
		if (music->isLoaded && ::IsMusicStreamPlaying(music->music))
		{
#ifdef EDITOR
			// Get editor settings from context
			auto& debugSettings = context.GetEditor().GetSettings().debugRender;
			if (debugSettings.audio)
			{
#endif
				::UpdateMusicStream(music->music);
#ifdef EDITOR
			}
#endif
		}
	}
}
