#pragma once

#include <functional>
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
// A font's cache identity - path alone isn't enough, the same file can be loaded at multiple sizes and each
// size needs its own glyph atlas/FontResource. Replaces the old "<path>_<size>" string-encoding hack (fragile:
// a font file literally named "..._2.ttf" had its own "_2" misread as a font size - see git history).
struct FontKey
{
	std::string path;
	int size;

	bool operator==(const FontKey& other) const
	{
		return path == other.path && size == other.size;
	}
};

inline std::string ToDisplayName(const FontKey& key)
{
	return key.path + " (" + std::to_string(key.size) + "pt)";
}
}  // namespace Resource
}  // namespace Struktur

// Must come before FontPool's own declaration below - it instantiates ResourcePool<FontResource, FontKey>, whose
// std::unordered_map<FontKey, ...> member requires this specialization to already be visible (an explicit
// specialization appearing after the primary template's first implicit use is ill-formed).
namespace std
{
template <>
struct hash<Struktur::Resource::FontKey>
{
	size_t operator()(const Struktur::Resource::FontKey& key) const
	{
		return hash<std::string>()(key.path) ^ (hash<int>()(key.size) << 1);
	}
};
}  // namespace std

namespace Struktur
{
namespace Resource
{
class FontPool : public GpuResourcePool<FontResource, FontKey>
{
public:
	FontPool()
	    : GpuResourcePool<FontResource, FontKey>(32 * 1024 * 1024)
	{
	}  // 32MB for textures

protected:
	FontResource* LoadResource(GameContext& context, const FontKey& key) override;
};
}  // namespace Resource
}  // namespace Struktur
