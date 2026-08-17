#include "MusicResource.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"

Struktur::Resource::MusicResource::MusicResource(const std::string& filePath)
    : CpuResource(filePath)
{
}

Struktur::Resource::MusicResource::MusicResource(MusicResource&& other) noexcept
    : CpuResource(std::move(other)),
      audio(other.audio),
      track(other.track),
      buffer(std::move(other.buffer))
{
	other.audio = nullptr;
	other.track = nullptr;
}

Struktur::Resource::MusicResource& Struktur::Resource::MusicResource::operator=(MusicResource&& other) noexcept
{
	if (this != &other)
	{
		UnloadFromHardware();
		UnloadFromDisk();
		CpuResource::operator=(std::move(other));
		audio       = other.audio;
		track       = other.track;
		buffer      = std::move(other.buffer);
		other.audio = nullptr;
		other.track = nullptr;
	}
	return *this;
}

Struktur::Resource::MusicResource::~MusicResource()
{
	UnloadFromHardware();
	UnloadFromDisk();
}

bool Struktur::Resource::MusicResource::LoadFromDisk(GameContext& context)
{
	if (isLoaded)
	{
		return true;
	}

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());
	buffer = std::move(result.value);

	// MIX_LoadAudio_IO() makes its own copy; `buffer` is kept regardless, matching its prior lifetime.
	MIX_Mixer* mixer = context.GetMixer().GetHandle();

	SDL_IOStream* io = ::SDL_IOFromConstMem(buffer.data(), buffer.size());
	audio            = ::MIX_LoadAudio_IO(mixer, io, /*predecode=*/false, /*closeio=*/true);
	if (!audio)
	{
		DEBUG_ERROR("Failed to decode music stream '%s': %s", filePath.c_str(), SDL_GetError());
		return false;
	}

	isLoaded = true;
	DEBUG_INFO("Loaded music stream: %s", filePath);
	return true;
}

void Struktur::Resource::MusicResource::UnloadFromDisk()
{
	// UnloadFromHardware() must run first - destroying audio while a track still uses it is unsafe.
	if (audio)
	{
		::MIX_DestroyAudio(audio);
		audio = nullptr;
	}
	isLoaded = false;
}

bool Struktur::Resource::MusicResource::LoadToHardware(GameContext& context)
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
	if (!track || !::MIX_SetTrackAudio(track, audio))
	{
		DEBUG_ERROR("Failed to load music to audio hardware: %s: %s", filePath.c_str(), SDL_GetError());
		return false;
	}

	::MIX_SetTrackLoops(track, -1);  // matches raylib's previous default-to-looping behaviour

	DEBUG_INFO("Loaded music stream to audio hardware: %s", filePath);
	return true;
}

void Struktur::Resource::MusicResource::UnloadFromHardware()
{
	if (track)
	{
		::MIX_DestroyTrack(track);
		track = nullptr;
		DEBUG_INFO("Unloaded music stream from audio hardware: %s", filePath);
	}
}

size_t Struktur::Resource::MusicResource::GetMemoryUsage() const
{
	if (!isLoaded || !audio)
	{
		return 0;
	}

	SDL_AudioSpec spec;
	if (!::MIX_GetAudioFormat(audio, &spec))
	{
		return 0;
	}

	Sint64 frames = ::MIX_GetAudioDuration(audio);
	if (frames < 0)
	{
		return 0;  // MIX_DURATION_UNKNOWN / MIX_DURATION_INFINITE
	}

	return static_cast<size_t>(frames) * SDL_AUDIO_FRAMESIZE(spec);
}

bool Struktur::Resource::MusicResource::IsHardwareReady() const
{
	return track != nullptr;
}
