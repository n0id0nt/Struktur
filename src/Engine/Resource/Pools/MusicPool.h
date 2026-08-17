#pragma once

#include <string>

#include "Engine/Resource/MusicResource.h"
#include "Engine/Resource/Pools/ResourcePool.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
class MusicPool : public ResourcePool<MusicResource>
{
protected:
	MusicResource* LoadResource(GameContext& context, const std::string& filePath) override;
};
}  // namespace Resource
}  // namespace Struktur
