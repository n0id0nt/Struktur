#pragma once

#include <string>
#include <unordered_map>

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/Util/MathUtil.h"

namespace Struktur
{
namespace UI
{
// A single RGBA texture (loaded like any other TextureResource - see UITexture) shared by many named icons,
// each occupying its own pixel-space sub-rect within it - the [icon=name] counterpart to FontResource's
// glyph atlas, except icons keep their own color (drawn via UIRenderer::DrawTexturedRectRegion, tinted white
// so the source pixels pass through unmodified - see UIRichLabel::RenderLine) rather than being a
// single-channel coverage atlas like the font path.
//
// Deliberately just a name->rect table over a plain TextureResource - no packer/loader pipeline, no GPU
// resource pool integration of its own. Callers position icons by hand via AddIcon; a data-driven atlas
// format/tool is a natural later addition if a hand-authored rect list ever becomes unwieldy.
class IconAtlas
{
private:
	Resource::ResourcePtr<Resource::TextureResource> m_texture;
	std::unordered_map<std::string, Util::Math::Rect> m_icons;  // name -> pixel-space rect within m_texture

public:
	void SetTexture(Resource::ResourcePtr<Resource::TextureResource> texture)
	{
		m_texture = std::move(texture);
	}
	void AddIcon(const std::string& name, const Util::Math::Rect& sourceRectPixels)
	{
		m_icons[name] = sourceRectPixels;
	}

	bool HasTexture() const
	{
		return (bool)m_texture;
	}
	const Resource::ResourcePtr<Resource::TextureResource>& GetTexture() const
	{
		return m_texture;
	}

	// Returns false (leaving outRect untouched) if name was never registered via AddIcon - callers treat an
	// unresolved icon as "draw nothing for this run" rather than a hard error (see UIRichLabel::RenderLine),
	// matching how an unset font falls back gracefully rather than crashing.
	bool TryGetIconRect(const std::string& name, Util::Math::Rect& outRect) const
	{
		auto it = m_icons.find(name);
		if (it == m_icons.end())
		{
			return false;
		}
		outRect = it->second;
		return true;
	}
};
}  // namespace UI
}  // namespace Struktur
