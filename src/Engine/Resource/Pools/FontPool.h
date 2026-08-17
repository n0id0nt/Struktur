#pragma once

#include <string>

#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/Pools/GpuResourcePool.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
class FontPool : public GpuResourcePool<FontResource>
{
private:
	int defaultFontSize = 32;

public:
	FontPool()
	    : GpuResourcePool<FontResource>(32 * 1024 * 1024)
	{
	}  // 32MB for textures

protected:
	FontResource* LoadResource(GameContext& context, const std::string& resourceString) override;
};
}  // namespace Resource
}  // namespace Struktur
