#include "MusicPool.h"

Struktur::Resource::MusicResource* Struktur::Resource::MusicPool::LoadResource(GameContext& context,
                                                                               const std::string& filePath)
{
	auto* music = new MusicResource(filePath);

	if (!music->LoadFromDisk(context))
	{
		delete music;
		return nullptr;
	}

	return music;
}
