#include "FontResource.h"

#include "Engine/Core/FileSystem.h"

Struktur::Resource::FontResource::FontResource(const std::string& filePath, int size)
	: GpuResource(filePath), m_fontLoaded(false), m_fontSize(size),
	m_codepoints(nullptr), m_codepointCount(0)
{
	font.texture.id = 0;
	font.baseSize = 0;
	font.glyphCount = 0;
	font.glyphs = nullptr;
	font.recs = nullptr;
}

Struktur::Resource::FontResource::~FontResource()
{
	UnloadFromGpu();
	UnloadFromDisk();
}

bool Struktur::Resource::FontResource::LoadFromDisk()
{
	if (m_fontLoaded) return true;

	// For default fonts, we can load directly
	if (filePath.empty() || filePath == "default")
	{
		font = ::GetFontDefault();
		m_fontLoaded = true;
		isLoaded = true;
		DEBUG_INFO(std::format("Loaded default font: {}", filePath).c_str());
		return true;
	}

	// Load custom font - this loads both disk data and creates GPU texture
	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());

	const char* ext = ::GetFileExtension(filePath.c_str());
	font = ::LoadFontFromMemory(ext, result.value.data(), result.value.size(), m_fontSize, m_codepoints, m_codepointCount);

	if (font.texture.id == 0)
	{
		BREAK_MSG(std::format("Failed to load font: {}", filePath).c_str());
		return false;
	}

	m_fontLoaded = true;
	isLoaded = true;
	DEBUG_INFO(std::format("Loaded font from disk: {} (size: {})", filePath, m_fontSize).c_str());
	return true;
}

void Struktur::Resource::FontResource::UnloadFromDisk()
{
	if (m_fontLoaded && font.texture.id != 0)
	{
		// Only unload if it's not the default font
		if (filePath != "default" && !filePath.empty())
		{
			::UnloadFont(font);
		}
		font.texture.id = 0;
		font.baseSize = 0;
		font.glyphCount = 0;
		font.glyphs = nullptr;
		font.recs = nullptr;
		m_fontLoaded = false;
	}
	isLoaded = false;
}

bool Struktur::Resource::FontResource::LoadToGpu()
{
	// For fonts, loading from disk already creates the GPU texture
	// So we just need to ensure disk loading happened
	return LoadFromDisk();
}

void Struktur::Resource::FontResource::UnloadFromGpu()
{
	// For fonts, GPU and disk data are tied together
	// We can't separate them like with textures
	// So GPU unload is the same as full unload
	if (m_fontLoaded && font.texture.id != 0) {
		if (filePath != "default" && !filePath.empty())
		{
			::UnloadFont(font);
		}
		font.texture.id = 0;
		m_fontLoaded = false;
	}
}

bool Struktur::Resource::FontResource::IsGpuResourceValid() const
{
	return font.texture.id != 0 && font.baseSize > 0;
}

size_t Struktur::Resource::FontResource::GetMemoryUsage() const
{
	if (!m_fontLoaded) return 0;

	// Estimate: glyph data + texture data
	size_t glyphDataSize = font.glyphCount * (sizeof(GlyphInfo) + sizeof(Rectangle));
	size_t textureSize = font.texture.width * font.texture.height * 4; // RGBA
	return glyphDataSize + textureSize;
}

size_t Struktur::Resource::FontResource::GetGpuMemoryUsage() const
{
	return m_fontLoaded ? (font.texture.width * font.texture.height * 4) : 0;
}

void Struktur::Resource::FontResource::SetCodepoints(int* customCodepoints, int count)
{
	m_codepoints = customCodepoints;
	m_codepointCount = count;
}

void Struktur::Resource::FontResource::SetFontSize(int size)
{
	m_fontSize = size;
}

int Struktur::Resource::FontResource::GetFontSize()
{
	return m_fontSize;
}

void Struktur::Resource::FontResource::DrawText(const std::string& text, Vector2 position, float fontSize, Color color) const
{
	if (IsGpuReady())
	{
		::DrawTextEx(font, text.c_str(), position, fontSize, 1.0f, color);
	}
}

int Struktur::Resource::FontResource::GetBaseSize() const
{
	return font.baseSize;
}

int Struktur::Resource::FontResource::GetGlyphCount() const
{
	return font.glyphCount;
}

Struktur::Resource::FontResource* Struktur::Resource::FontPool::LoadResource(const std::string& resourceString)
{
	std::string filePath;
	int fontSize = defaultFontSize;
	size_t underscorePos = resourceString.find_last_of('_');
	if (underscorePos != std::string::npos)
	{
		// Parse the number after the underscore
		int specifiedSize = std::stoi(resourceString.substr(underscorePos + 1));
		if (specifiedSize > 0 && specifiedSize <= 256)
		{  // Reasonable size limits
			fontSize = specifiedSize;

			// Remove size from path - get base name before underscore
			filePath = resourceString.substr(0, underscorePos);
		}
	}

	auto* font = new FontResource(filePath, fontSize);

	if (!font->LoadFromDisk())
	{
		delete font;
		return nullptr;
	}

	AddGpuResource(font);
	return font;
}
