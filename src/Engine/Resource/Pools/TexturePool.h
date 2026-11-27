#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Resource/Pools/GpuResourcePool.h"
#include "Engine/Resource/TextureResource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
class TexturePool : public GpuResourcePool<TextureResource>
{
public:
	TexturePool()
	    : GpuResourcePool<TextureResource>(256 * 1024 * 1024)
	{
	}  // 256MB for textures

	// Decodes every listed texture that isn't loaded yet in parallel on the job system, and waits. The next
	// GetResource() for each one then skips the file read and decode. Call before a batch of GetResource() calls
	// (e.g. a level's tilesets); with zero workers it simply decodes them in turn.
	void Prefetch(GameContext& context, const std::vector<std::string>& filePaths);
	size_t GetPrefetchedCount() const
	{
		return m_prefetched.size();
	}

protected:
	TextureResource* LoadResource(GameContext& context, const std::string& filePath) override;

private:
	std::unordered_map<std::string, TextureResource::DecodedImage> m_prefetched;  // main thread only
};
}  // namespace Resource
}  // namespace Struktur
