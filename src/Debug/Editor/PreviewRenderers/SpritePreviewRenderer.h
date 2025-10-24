#pragma once

#include "PreviewRenderer.h"
#include <raylib.h>

namespace Struktur
{
    namespace Component
    {
        struct Sprite;
    }
}

namespace Struktur::Debug
{
    class SpritePreviewRenderer : public IPreviewRenderer
    {
    public:
        SpritePreviewRenderer(const Component::Sprite* sprite, const std::string& name)
            : m_sprite(sprite)
            , m_name(name)
        {
        }
        
        void Render(GameContext& context, const ImVec2& availableSize) override;
        std::string GetPreviewName() const override { return m_name; }
        
    private:
        const Component::Sprite* m_sprite;
        std::string m_name;
    };
}
