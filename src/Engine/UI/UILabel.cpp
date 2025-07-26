#include "UILabel.h"

Struktur::UI::UILabel::UILabel(const glm::vec2 &pos, const std::string &labelText, float fontSz)
    : UIElement(pos, {0, 0}), m_text(labelText), m_textColor(BLACK),
    m_alignment(TextAlignment::LEFT), m_wordWrap(false), m_fontSize(fontSz)
{
    
    m_font = ::GetFontDefault();
    
    // Auto-size based on text
    ::Vector2 textSize = ::MeasureTextEx(m_font, m_text.c_str(), m_fontSize, 1.0f);
    SetSize({textSize.x + 10, textSize.y + 5}); // Add some padding
    
    // Labels are typically not focusable
    m_focusable = false;
    m_backgroundColor = BLANK; // Transparent by default
    m_borderWidth = 0.0f;
}

void Struktur::UI::UILabel::SetText(const std::string &newText)
{
    m_text = newText;
    // Recalculate size
    ::Vector2 textSize = ::MeasureTextEx(m_font, m_text.c_str(), m_fontSize, 1.0f);
    SetSize({textSize.x + 10, textSize.y + 5});
}

void Struktur::UI::UILabel::Update(GameContext &context)
{
    UpdateChildren(context);
}

void Struktur::UI::UILabel::Render(GameContext &context)
{
    // Draw background if not transparent
    if (m_backgroundColor.a > 0)
    {
        ::DrawRectangleRec(m_bounds, m_backgroundColor);
    }

    // Draw border if needed
    if (m_borderWidth > 0)
    {
        ::DrawRectangleLinesEx(m_bounds, m_borderWidth, m_borderColor);
    }

    // Calculate text position based on alignment
    ::Vector2 textPos = {m_bounds.x + 5, m_bounds.y + 2.5f};
    ::Vector2 textSize = ::MeasureTextEx(m_font, m_text.c_str(), m_fontSize, 1.0f);

    switch (m_alignment)
    {
        case TextAlignment::CENTER:
            textPos.x = m_bounds.x + (m_bounds.width - textSize.x) / 2.0f;
            break;
        case TextAlignment::RIGHT:
            textPos.x = m_bounds.x + m_bounds.width - textSize.x - 5;
            break;
        case TextAlignment::LEFT:
        default:
            // Already set above
            break;
    }

    // Draw text
    ::DrawTextEx(m_font, m_text.c_str(), textPos, m_fontSize, 1.0f, m_textColor);

    RenderChildren(context);
}