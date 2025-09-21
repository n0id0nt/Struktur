#pragma once

#include "Engine/Core/Resource/ResourcePtr.h"
#include "Engine/Core/Resource/TextureResource.h"

namespace Struktur
{
	namespace UI
	{
        //=============================================================================
        // UIPanel - Container element
        //=============================================================================
        class UITexture
        {
        private:
            Core::Resource::ResourcePtr<Core::Resource::TextureResource> m_backgroundTexture; // TODO replace this with resoruce loader texture
            int columns, rows;
            int index;

        public:
            UITexture(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize, const glm::vec2& relativeSize);

            void SetBackgroundColor(::Color color);
            void SetBackgroundTexture(Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture);
            void ClearBackgroundTexture();

            void Update(GameContext& context) override;
            void Render(GameContext& context) override;
        };
    }
}