#pragma once
#include <cstdint>
#include <string>
#include <vector>

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
// SDL3_mixer music - CPU resource (streaming audio)
class MusicResource : public CpuResource
{
   public:
	MIX_Audio* audio = nullptr;
	// Persistent playback instance, created once in LoadToHardware() and reused thereafter.
	MIX_Track* track = nullptr;
	std::vector<uint8_t> buffer;  // TODO sore this in the music pool instead so that the memory can more easily be
	                              // accessed and retrieved

	MusicResource(const std::string& filePath);
	~MusicResource();

	bool LoadFromDisk(GameContext& context) override;
	void UnloadFromDisk() override;
	bool LoadToHardware(GameContext& context) override;
	void UnloadFromHardware() override;

	bool IsHardwareReady() const override;

	size_t GetMemoryUsage() const override;
};
}  // namespace Resource
}  // namespace Struktur
