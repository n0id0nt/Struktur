#pragma once
#include <format>
#include <string>

#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"
#include "raylib.h"

namespace Struktur
{
namespace Resource
{
// Raylib music - CPU resource (streaming audio)
class MusicResource : public CpuResource
{
   public:
	Music music;
	std::vector<uint8_t> buffer;  // TODO sore this in the music pool instead so that the memory can more easily be
	                              // accessed and retrieved

	MusicResource(const std::string& filePath);
	~MusicResource();

	bool LoadFromDisk() override;
	void UnloadFromDisk() override;
	bool LoadToHardware() override;

	bool IsHardwareReady() const override;

	size_t GetMemoryUsage() const override;
};

class MusicPool : public ResourcePool<MusicResource>
{
   protected:
	MusicResource* LoadResource(const std::string& filePath) override;
};
}  // namespace Resource
}  // namespace Struktur
