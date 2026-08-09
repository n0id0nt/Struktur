#pragma once

#include <memory>

struct MIX_Mixer;

namespace Struktur
{
namespace Audio
{
// Owns the SDL3_mixer mixer device; replaces raylib's raudio (InitAudioDevice/CloseAudioDevice).
class Mixer
{
   public:
	Mixer();
	~Mixer();

	Mixer(const Mixer&)            = delete;
	Mixer& operator=(const Mixer&) = delete;

	MIX_Mixer* GetHandle() const;

	// Master volume applied on top of every track's own gain - 0 is silent, 1 is unattenuated.
	void SetMasterVolume(float volume) const;
	float GetMasterVolume() const;

   private:
	// Lets unique_ptr manage the opaque MIX_Mixer without needing SDL_mixer.h in this header.
	struct MixerDeleter
	{
		void operator()(MIX_Mixer* mixer) const;
	};
	std::unique_ptr<MIX_Mixer, MixerDeleter> m_mixer;
};
}  // namespace Audio
}  // namespace Struktur
