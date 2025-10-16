#pragma once

#include "Engine/UI/UIElement.h"

#include "Engine/Resource/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"

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
            Resource::ResourcePtr<Resource::TextureResource> m_backgroundTexture; // TODO replace this with resoruce loader texture
            bool m_hasBackgroundTexture;

        public:
            UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize, const glm::vec2& relativeSize);

            void SetBackgroundColor(::Color color);
            void SetBackgroundTexture(Resource::ResourcePtr<Resource::TextureResource> texture);
            void ClearBackgroundTexture();

            void Update(GameContext& context) override;
            void Render(GameContext& context) override;
        };
    }
}