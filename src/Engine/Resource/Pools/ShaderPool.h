#pragma once

#include <functional>
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
// A shader program's cache identity - a vertex and fragment shader together. Replaces the old "<vs>,<fs>"
// string-encoding hack (fragile: a path containing a literal comma would silently split wrong).
struct ShaderKey
{
	std::string vsFilePath;
	std::string fsFilePath;

	bool operator==(const ShaderKey& other) const
	{
		return vsFilePath == other.vsFilePath && fsFilePath == other.fsFilePath;
	}
};

inline std::string ToDisplayName(const ShaderKey& key)
{
	return key.vsFilePath + " + " + key.fsFilePath;
}
}  // namespace Resource
}  // namespace Struktur

// Must come before ShaderPool's own declaration below - it instantiates ResourcePool<ShaderResource, ShaderKey>,
// whose std::unordered_map<ShaderKey, ...> member requires this specialization to already be visible (an
// explicit specialization appearing after the primary template's first implicit use is ill-formed).
namespace std
{
template <>
struct hash<Struktur::Resource::ShaderKey>
{
	size_t operator()(const Struktur::Resource::ShaderKey& key) const
	{
		return hash<std::string>()(key.vsFilePath) ^ (hash<std::string>()(key.fsFilePath) << 1);
	}
};
}  // namespace std

namespace Struktur
{
namespace Resource
{
class ShaderPool : public GpuResourcePool<ShaderResource, ShaderKey>
{
public:
	ShaderPool()
	    : GpuResourcePool<ShaderResource, ShaderKey>(1 * 1024 * 1024)
	{
	}  // 1MB for textures

protected:
	ShaderResource* LoadResource(GameContext& context, const ShaderKey& key) override;
};
}  // namespace Resource
}  // namespace Struktur
