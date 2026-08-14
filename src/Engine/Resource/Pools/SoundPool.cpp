#include "SoundPool.h"

Struktur::Resource::SoundResource* Struktur::Resource::SoundPool::LoadResource(GameContext& context,
                                                                               const std::string& filePath)
{
	auto* sound = new SoundResource(filePath);

	if (!sound->LoadFromDisk(context))
	{
		delete sound;
		return nullptr;
	}

	return sound;
}

bool Struktur::Resource::SoundPool::EnsureResourceReady(GameContext& context, const std::string& filePath)
{
	auto it = m_loadedResources.find(filePath);
	if (it == m_loadedResources.end())
	{
		return false;
	}

	SoundResource* sound = it->second.resource;

	// Load from disk first
	if (!sound->isLoaded && !sound->LoadFromDisk(context))
	{
		return false;
	}

	// Then load to audio hardware
	return sound->LoadToHardware(context);
}
