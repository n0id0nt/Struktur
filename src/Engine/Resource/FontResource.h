#pragma once

#include <format>
#include <string>

#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"
#include "raylib.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// Raylib font - GPU resource (contains texture atlas)
class FontResource : public GpuResource
{
   private:
	bool m_fontLoaded;
	int m_fontSize;
	int* m_codepoints;  // Custom codepoints for font loading
	int m_codepointCount;

   public:
	Font font;

	FontResource(const std::string& filePath, int size);
	~FontResource();

	bool LoadFromDisk(GameContext& context) override;
	void UnloadFromDisk() override;
	bool LoadToGpu() override;
	void UnloadFromGpu() override;

	bool IsGpuResourceValid() const override;

	size_t GetMemoryUsage() const override;
	size_t GetGpuMemoryUsage() const override;

	// Font-specific configuration methods
	void SetCodepoints(int* customCodepoints, int count);
	void SetFontSize(int size);
	int GetFontSize();

	// Convenience methods for text rendering
	void DrawText(const std::string& text, Vector2 position, float fontSize, Color color) const;

	int GetBaseSize() const;
	int GetGlyphCount() const;
};

// Specialized pools
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
