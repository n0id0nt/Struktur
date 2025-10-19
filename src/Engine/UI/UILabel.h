#pragma once

#include "Engine/UI/UIElement.h"
#include "Engine/Resource/ResourcePtr.h"
#include "Engine/Resource/FontResource.h"

namespace Struktur
{
    class GameContext;
    
namespace UI
{
        enum class TextAlignment {
            LEFT, CENTER, RIGHT, JUSTIFY
        };

        enum class TextWrapping {
            NONE, WORD_WRAP, CHARACTER_WRAP
        };

        //=============================================================================
        // UILabel - Text display element
        //=============================================================================
        class UILabel : public UIElement
        {
        private:
            std::string m_text;
            Resource::ResourcePtr<Resource::FontResource> m_font;
            ::Color m_textColor;
            TextAlignment m_alignment;
            TextWrapping m_wrapping;
            float m_fontSize;

        public:
            UILabel(GameContext& context, const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const std::string& labelText, float fontSz = 20.0f);

            void SetText(const std::string& newText);

            void SetFont(Resource::ResourcePtr<Resource::FontResource> newFont) { m_font = std::move(newFont); }
            void SetTextColor(::Color color) { m_textColor = color; }
            void SetAlignment(TextAlignment align) { m_alignment = align; }
            void SetFontSize(float size) { m_fontSize = size; }
            void SetWordWrap(TextWrapping wrap) { m_wrapping = wrap; }

            const std::string& GetText() const { return m_text; }

            void Update(GameContext& context) override;
            void Render(GameContext& context) override;

            std::vector<std::string> WrapText(const std::string& text, float maxWidth);
            void RenderJustifiedLine(const std::string& line, ::Vector2 pos, float targetWidth, bool isLastLine);
            void RenderWrappedText(const std::string& text, ::Vector2 startPos, float lineHeight);

            ::Rectangle GetFormattedTextBounds() const;
            ::Vector2 GetFormattedTextSize() const;
            float GetLineHeight() const;
        };
    }
}