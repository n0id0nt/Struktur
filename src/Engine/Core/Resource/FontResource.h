#pragma once
#include <string>
#include <format>
#include "raylib.h"

#include "Engine/Core/Resource/Resource.h"
#include "Engine/Core/Resource/ResourcePool.h"
#include "Engine/Core/Resource/ResourcePtr.h"

namespace Struktur
{
	namespace Core
	{
		namespace Resource
		{
            // Raylib font - GPU resource (contains texture atlas)
            class FontResource : public GpuResource
            {
            private:
                bool m_fontLoaded;
                int m_fontSize;
                int* m_codepoints;      // Custom codepoints for font loading
                int m_codepointCount;
                
            public:
                Font font;
                
                FontResource(const std::string& filePath, int size = 32) 
                    : GpuResource(filePath), m_fontLoaded(false), m_fontSize(size), 
                    m_codepoints(nullptr), m_codepointCount(0)
                {
                    font.texture.id = 0;
                    font.baseSize = 0;
                    font.glyphCount = 0;
                    font.glyphs = nullptr;
                    font.recs = nullptr;
                }
                
                ~FontResource()
                {
                    UnloadFromGpu();
                    UnloadFromDisk();
                }
                
                bool LoadFromDisk() override
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
                    font = ::LoadFontEx(filePath.c_str(), m_fontSize, m_codepoints, m_codepointCount);
                    
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
                
                void UnloadFromDisk() override {
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
                
                bool LoadToGpu() override
                {
                    // For fonts, loading from disk already creates the GPU texture
                    // So we just need to ensure disk loading happened
                    return LoadFromDisk();
                }
                
                void UnloadFromGpu() override
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
                
                bool IsGpuResourceValid() const override
                {
                    return font.texture.id != 0 && font.baseSize > 0;
                }
                
                size_t GetMemoryUsage() const override
                {
                    if (!m_fontLoaded) return 0;
                    
                    // Estimate: glyph data + texture data
                    size_t glyphDataSize = font.glyphCount * (sizeof(GlyphInfo) + sizeof(Rectangle));
                    size_t textureSize = font.texture.width * font.texture.height * 4; // RGBA
                    return glyphDataSize + textureSize;
                }
                
                size_t GetGpuMemoryUsage() const override
                {
                    return m_fontLoaded ? (font.texture.width * font.texture.height * 4) : 0;
                }
                
                // Font-specific configuration methods
                void SetCodepoints(int* customCodepoints, int count)
                {
                    m_codepoints = customCodepoints;
                    m_codepointCount = count;
                }
                
                void SetFontSize(int size)
                {
                    m_fontSize = size;
                }
                
                // Convenience methods for text rendering
                void DrawText(const std::string& text, Vector2 position, float fontSize, Color color) const
                {
                    if (IsGpuReady())
                    {
                        ::DrawTextEx(font, text.c_str(), position, fontSize, 1.0f, color);
                    }
                }
                
                int GetBaseSize() const
                {
                    return font.baseSize;
                }
                
                int GetGlyphCount() const
                {
                    return font.glyphCount;
                }
            };

            // Specialized pools
			class FontPool : public GpuResourcePool<FontResource>
			{
            private:
                int defaultFontSize = 32;
			public:
				FontPool() : GpuResourcePool<FontResource>(32 * 1024 * 1024) {} // 256MB for textures
				
			protected:
				FontResource* LoadResource(const std::string& resourceString) override
                {
                    std::string filePath;
                    int fontSize = defaultFontSize;
                    size_t underscorePos = resourceString.find_last_of('_');
                    if (underscorePos != std::string::npos)
                    {
                        try
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
                        catch (const std::exception&)
                        {
                            // If parsing fails, use original name (maybe underscore wasn't size)
                            filePath = resourceString;
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
			};
        }
    }
}
