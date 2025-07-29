#pragma once

#include "Engine/UI/UIElement.h"

#include "Engine/Core/Resource/ResourcePtr.h"
#include "Engine/Core/Resource/TextureResource.h"

namespace Struktur
{
	namespace UI
	{
        //=============================================================================
        // UIPanel - Container element
        //=============================================================================
        class UIPanel : public UIElement
        {
        private:
            ::Color m_panelColor;
            Core::Resource::ResourcePtr<Core::Resource::TextureResource> m_backgroundTexture; // TODO replace this with resoruce loader texture
            bool m_hasBackgroundTexture;

        public:
            UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize, const glm::vec2& relativeSize);

            void SetBackgroundColor(::Color color);
            void SetBackgroundTexture(Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture);
            void ClearBackgroundTexture();

            void Update(GameContext& context) override;
            void Render(GameContext& context) override;
        };
    }
}