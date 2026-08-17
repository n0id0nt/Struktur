#pragma once

#include "Engine/Resource/TextureResource.h"
#include "PreviewRenderer.h"

namespace Struktur::Debug
{
class TexturePreviewRenderer : public IPreviewRenderer
{
public:
	TexturePreviewRenderer(const Resource::ResourcePtr<Resource::TextureResource>& texture, const std::string& name)
	    : m_texture(texture),
	      m_name(name)
	{
	}

	void Render(GameContext& context, const ImVec2& availableSize) override;
	std::string GetPreviewName() const override
	{
		return m_name;
	}

private:
	Resource::ResourcePtr<Resource::TextureResource> m_texture;
	std::string m_name;
};
}  // namespace Struktur::Debug
