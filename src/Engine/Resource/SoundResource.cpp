#include "SoundResource.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"

Struktur::Resource::SoundResource::SoundResource(const std::string& filePath)
    : CpuResource(filePath)
{
}

Struktur::Resource::SoundResource::SoundResource(SoundResource&& other) noexcept
    : CpuResource(std::move(other)),
      m_audio(other.m_audio),
      track(other.track)
{
	other.m_audio = nullptr;
	other.track   = nullptr;
}

Struktur::Resource::SoundResource& Struktur::Resource::SoundResource::operator=(SoundResource&& other) noexcept
{
	if (this != &other)
	{
		UnloadFromHardware();
		UnloadFromDisk();
		CpuResource::operator=(std::move(other));
		m_audio       = other.m_audio;
		track         = other.track;
		other.m_audio = nullptr;
		other.track   = nullptr;
	}
	return *this;
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
	DEBUG_INFO("Loaded sound from disk: %s", filePath);
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

	DEBUG_INFO("Loaded sound to audio hardware: %s", filePath);
	return true;
}

void Struktur::Resource::SoundResource::UnloadFromHardware()
{
	if (track)
	{
		::MIX_DestroyTrack(track);
		track = nullptr;
		DEBUG_INFO("Unloaded sound from audio hardware: %s", filePath);
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
