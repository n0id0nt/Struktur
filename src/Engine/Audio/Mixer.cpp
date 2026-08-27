#include "Mixer.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "Debug/Assertions.h"

namespace Struktur
{
namespace Audio
{
void Mixer::MixerDeleter::operator()(MIX_Mixer* mixer) const
{
	if (mixer)
	{
		::MIX_DestroyMixer(mixer);
	}
}

Mixer::Mixer()
{
	if (!::MIX_Init())
	{
		DEBUG_ERROR("Failed to initialise SDL3_mixer: %s", SDL_GetError());
	}
	else
	{
		MIX_Mixer* mixer = ::MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
		if (!mixer)
		{
			DEBUG_ERROR("Failed to create SDL3_mixer device: %s", SDL_GetError());
		}
		else
		{
			DEBUG_INFO("SDL3_mixer mixer device created");
		}
		m_mixer.reset(mixer);
	}
}

Mixer::~Mixer()
{
	m_mixer.reset();  // destroy the mixer (and its tracks/groups) before MIX_Quit()
	::MIX_Quit();
}

MIX_Mixer* Mixer::GetHandle() const
{
	return m_mixer.get();
}

void Mixer::SetMasterVolume(float volume) const
{
	::MIX_SetMixerGain(m_mixer.get(), volume);
}

float Mixer::GetMasterVolume() const
{
	return ::MIX_GetMixerGain(m_mixer.get());
}
}  // namespace Audio
}  // namespace Struktur
