#include "SoundResource.h"

#include <format>

#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

Struktur::Resource::SoundResource::SoundResource(const std::string& filePath)
    : CpuResource(filePath)
{
}

Struktur::Resource::SoundResource::~SoundResource()
{
	UnloadFromHardware();
	UnloadFromDisk();
}

bool Struktur::Resource::SoundResource::LoadFromDisk(GameContext& context)
{
	if (isLoaded)
	{
		return true;
	}

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());

	// MIX_LoadAudio_IO() makes its own copy, so `result.value` only needs to live for this call.
	MIX_Mixer* mixer = context.GetMixer().GetHandle();

	SDL_IOStream* io = ::SDL_IOFromConstMem(result.value.data(), result.value.size());
	m_audio          = ::MIX_LoadAudio_IO(mixer, io, /*predecode=*/false, /*closeio=*/true);
	if (!m_audio)
	{
		DEBUG_ERROR("Failed to decode sound '%s': %s", filePath.c_str(), SDL_GetError());
		return false;
	}

	isLoaded = true;
	DEBUG_INFO(std::format("Loaded sound from disk: {}", filePath).c_str());
	return true;
}

void Struktur::Resource::SoundResource::UnloadFromDisk()
{
	// UnloadFromHardware() must run first - destroying m_audio while a track still uses it is unsafe.
	if (m_audio)
	{
		::MIX_DestroyAudio(m_audio);
		m_audio = nullptr;
	}
	isLoaded = false;
}

bool Struktur::Resource::SoundResource::LoadToHardware(GameContext& context)
{
	if (!LoadFromDisk(context))
	{
		return false;
	}
	if (track)
	{
		return true;  // Already loaded
	}

	MIX_Mixer* mixer = context.GetMixer().GetHandle();

	track = ::MIX_CreateTrack(mixer);
	if (!track || !::MIX_SetTrackAudio(track, m_audio))
	{
		DEBUG_ERROR("Failed to load sound to audio hardware: %s: %s", filePath.c_str(), SDL_GetError());
		return false;
	}

	DEBUG_INFO(std::format("Loaded sound to audio hardware: {}", filePath).c_str());
	return true;
}

void Struktur::Resource::SoundResource::UnloadFromHardware()
{
	if (track)
	{
		::MIX_DestroyTrack(track);
		track = nullptr;
		DEBUG_INFO(std::format("Unloaded sound from audio hardware: {}", filePath).c_str());
	}
}

size_t Struktur::Resource::SoundResource::GetMemoryUsage() const
{
	if (!isLoaded || !m_audio)
	{
		return 0;
	}

	SDL_AudioSpec spec;
	if (!::MIX_GetAudioFormat(m_audio, &spec))
	{
		return 0;
	}

	Sint64 frames = ::MIX_GetAudioDuration(m_audio);
	if (frames < 0)
	{
		return 0;  // MIX_DURATION_UNKNOWN / MIX_DURATION_INFINITE
	}

	return static_cast<size_t>(frames) * SDL_AUDIO_FRAMESIZE(spec);
}

bool Struktur::Resource::SoundResource::IsHardwareReady() const
{
	return track != nullptr;
}

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
