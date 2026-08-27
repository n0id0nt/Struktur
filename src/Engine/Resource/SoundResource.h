#pragma once

#include <string>

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/ResourcePool.h"
#include "Engine/Resource/Resource.h"

struct MIX_Audio;
struct MIX_Track;

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// SDL3_mixer sound - CPU resource (uses audio hardware)
class SoundResource : public CpuResource
{
private:
	MIX_Audio* m_audio = nullptr;

public:
	// Persistent playback instance, created once in LoadToHardware() and reused thereafter.
	MIX_Track* track = nullptr;

	SoundResource(const std::string& filePath);
	SoundResource(SoundResource&& other) noexcept;
	SoundResource& operator=(SoundResource&& other) noexcept;
	~SoundResource();

	bool LoadFromDisk(GameContext& context) override;
	void UnloadFromDisk() override;
	bool LoadToHardware(GameContext& context) override;
	void UnloadFromHardware() override;

	bool IsHardwareReady() const override;
	size_t GetMemoryUsage() const override;
};
}  // namespace Resource
}  // namespace Struktur
