#pragma once

#include <string>

#include "Engine/Resource/Pools/GpuResourcePool.h"
#include "Engine/Resource/ShaderResource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
class ShaderPool : public GpuResourcePool<ShaderResource>
{
   public:
	ShaderPool()
	    : GpuResourcePool<ShaderResource>(1 * 1024 * 1024)
	{
	}  // 1MB for textures

   protected:
	ShaderResource* LoadResource(GameContext& context, const std::string& resourceString) override;
};
}  // namespace Resource
}  // namespace Struktur
