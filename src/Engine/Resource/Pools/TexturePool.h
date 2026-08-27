#pragma once

#include <string>

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

protected:
	TextureResource* LoadResource(GameContext& context, const std::string& filePath) override;
};
}  // namespace Resource
}  // namespace Struktur
