#include "SoundPool.h"

#ifdef EDITOR
#include <chrono>
#endif

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

bool Struktur::Resource::SoundPool::EnsureResourceReady(GameContext& context, ResourceHandle handle)
{
	SoundResource* sound = Resolve(handle);
	if (!sound)
	{
		return false;
	}

	// Load from disk first
	if (!sound->isLoaded && !sound->LoadFromDisk(context))
	{
		return false;
	}

	// Then load to audio hardware - LoadToHardware() is idempotent (returns early if already ready, see its own
	// comment), so this compares before/after rather than firing ReadyForUse on every call.
#ifdef EDITOR
	bool wasReady    = sound->IsHardwareReady();
	auto uploadStart = std::chrono::steady_clock::now();
#endif
	bool ready = sound->LoadToHardware(context);
#ifdef EDITOR
	if (ready && !wasReady && m_eventCallback)
	{
		double uploadSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - uploadStart).count();
		m_eventCallback(ResourceEventType::ReadyForUse, GetName(handle), sound->GetMemoryUsage(), uploadSeconds);
	}
#endif
	return ready;
}
