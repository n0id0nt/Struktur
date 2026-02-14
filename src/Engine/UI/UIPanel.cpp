#include "UIPanel.h"
#include "Engine/GameContext.h"

Struktur::UI::UIPanel::UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize), m_hasBackgroundTexture(false)
{                
    m_backgroundColor = LIGHTGRAY;
    m_focusable = false; // Panels typically don't receive focus
}


void Struktur::UI::UIPanel::SetBackgroundTexture(const Resource::ResourcePtr<Resource::TextureResource>& texture)
{
    m_backgroundTexture = texture;
    m_hasBackgroundTexture = true;
}

void Struktur::UI::UIPanel::ClearBackgroundTexture()
{
    m_hasBackgroundTexture = false;
    m_backgroundTexture = Resource::ResourcePtr<Resource::TextureResource>();
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
        Resource::TextureResource* texture = m_backgroundTexture.Get();
        if (!texture->IsGpuReady())
        {
            texture->LoadToGpu();
        }
        ::Rectangle srcRect = {0, 0, (float)texture->GetWidth(), (float)texture->GetHeight()};
        ::DrawTexturePro(texture->texture, srcRect, m_bounds, {0, 0}, 0.0f, WHITE);
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
