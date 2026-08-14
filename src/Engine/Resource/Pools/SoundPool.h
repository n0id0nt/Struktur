#pragma once

#include <string>

#include "Engine/Resource/Pools/ResourcePool.h"
#include "Engine/Resource/SoundResource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
class SoundPool : public ResourcePool<SoundResource>
{
   protected:
	SoundResource* LoadResource(GameContext& context, const std::string& filePath) override;

   public:
	bool EnsureResourceReady(GameContext& context, const std::string& filePath) override;
};
}  // namespace Resource
}  // namespace Struktur
