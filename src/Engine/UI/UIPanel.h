#pragma once

#include "Engine/UI/UIElement.h"

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
            ::Texture2D m_backgroundTexture; // TODO replace this with resoruce loader texture
            bool m_hasBackgroundTexture;

        public:
            UIPanel(const glm::vec2& pos, const glm::vec2& size);

            void SetBackgroundColor(::Color color);
            void SetBackgroundTexture(::Texture2D texture);
            void ClearBackgroundTexture();

            void Update(GameContext& context) override;
            void Render(GameContext& context) override;
        };
    }
}