#include "UIPanel.h"

Struktur::UI::UIPanel::UIPanel(const glm::vec2 &pos, const glm::vec2 &size)
    : UIElement(pos, size), m_panelColor(LIGHTGRAY), m_hasBackgroundTexture(false)
{                
    m_backgroundColor = m_panelColor;
    m_focusable = false; // Panels typically don't receive focus
}

void Struktur::UI::UIPanel::SetBackgroundColor(::Color color)
{
    m_panelColor = color;
    m_backgroundColor = color;
}

void Struktur::UI::UIPanel::SetBackgroundTexture(::Texture2D texture)
{
    m_backgroundTexture = texture;
    m_hasBackgroundTexture = true;
}

void Struktur::UI::UIPanel::ClearBackgroundTexture()
{
    m_hasBackgroundTexture = false;
}

void Struktur::UI::UIPanel::Update(GameContext &context)
{
    UpdateChildren(context);
}

void Struktur::UI::UIPanel::Render(GameContext &context)
{
    // Draw background
    if (m_hasBackgroundTexture)
    {
        // Scale texture to fit panel
        ::Rectangle srcRect = {0, 0, (float)m_backgroundTexture.width, (float)m_backgroundTexture.height};
        ::DrawTexturePro(m_backgroundTexture, srcRect, m_bounds, {0, 0}, 0.0f, WHITE);
    }
    else
    {
        ::DrawRectangleRec(m_bounds, m_backgroundColor);
    }

    // Draw border
    if (m_borderWidth > 0)
    {
        ::DrawRectangleLinesEx(m_bounds, m_borderWidth, m_borderColor);
    }

    RenderChildren(context);
}
