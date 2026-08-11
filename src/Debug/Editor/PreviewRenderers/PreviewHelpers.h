#pragma once

#include "PreviewWindow.h"
#include "ShaderPreviewRenderer.h"
#include "SpritePreviewRenderer.h"
#include "TexturePreviewRenderer.h"
// #include "CurvePreviewRenderer.h"
#include <memory>

#include "TextFilePreviewRenderer.h"
#include "UnknownFilePreviewRenderer.h"

namespace Struktur::Debug
{
// Helper functions to create and set preview renderers

inline void PreviewTexture(PreviewWindow* window, const Resource::ResourcePtr<Resource::TextureResource>& texture,
                           const std::string& name)
{
	if (!window)
	{
		return;
	}
	auto renderer = std::make_unique<TexturePreviewRenderer>(texture, name);
	window->SetPreviewRenderer(std::move(renderer));
}

inline void PreviewShader(PreviewWindow* window, const Component::Shader* shader, const std::string& name)
{
	if (!window)
	{
		return;
	}
	auto renderer = std::make_unique<ShaderPreviewRenderer>(shader, name);
	window->SetPreviewRenderer(std::move(renderer));
}

inline void PreviewSprite(PreviewWindow* window, const Component::Sprite* sprite, const std::string& name)
{
	if (!window)
	{
		return;
	}
	auto renderer = std::make_unique<SpritePreviewRenderer>(sprite, name);
	window->SetPreviewRenderer(std::move(renderer));
}

// inline void PreviewCurve(PreviewWindow* window,
//                          Util::Curve* curve,
//                          const std::string& name)
//{
//     if (!window) return;
//     auto renderer = std::make_unique<CurvePreviewRenderer>(curve, name);
//     window->SetPreviewRenderer(std::move(renderer));
// }

inline void PreviewTextFile(PreviewWindow* window, const std::string& filePath, const std::string& name)
{
	if (!window)
	{
		return;
	}
	auto renderer = std::make_unique<TextFilePreviewRenderer>(filePath, name);
	window->SetPreviewRenderer(std::move(renderer));
}

inline void PreviewUnknownFile(PreviewWindow* window, const std::string& fileName)
{
	if (!window)
	{
		return;
	}
	auto renderer = std::make_unique<UnknownFilePreviewRenderer>(fileName);
	window->SetPreviewRenderer(std::move(renderer));
}
}  // namespace Struktur::Debug
