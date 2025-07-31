#pragma once

#include "Engine/UI/UIElement.h"
#include "Engine/Core/Resource/ResourcePtr.h"
#include "Engine/Core/Resource/FontResource.h"

namespace Struktur
{
    class GameContext;
    
namespace UI
{
        enum class TextAlignment {
            LEFT, CENTER, RIGHT, JUSTIFY
        };

        //=============================================================================
        // UILabel - Text display element
        //=============================================================================
        class UILabel : public UIElement
        {
        private:
            std::string m_text;
            Core::Resource::ResourcePtr<Core::Resource::FontResource> m_font; // TODO Change this to UI a font from my resource manager
            ::Color m_textColor;
            TextAlignment m_alignment;
            bool m_wordWrap;
            float m_fontSize;

        public:
            UILabel(GameContext& context, const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const std::string& labelText, float fontSz = 20.0f);

            void SetText(const std::string& newText);

            void SetFont(Core::Resource::ResourcePtr<Core::Resource::FontResource> newFont) { m_font = std::move(newFont); }
            void SetTextColor(::Color color) { m_textColor = color; }
            void SetAlignment(TextAlignment align) { m_alignment = align; }
            void SetFontSize(float size) { m_fontSize = size; }
            void SetWordWrap(bool wrap) { m_wordWrap = wrap; }

            const std::string& GetText() const { return m_text; }

            void Update(GameContext& context) override;
            void Render(GameContext& context) override;
        };
    }
}