#pragma once

#include "EditorWindow.h"
#include "Debug/Editor/PreviewRenderers/PreviewRenderer.h"
#include <memory>

namespace Struktur::Debug
{
    class PreviewWindow : public EditorWindow
    {
    public:
        PreviewWindow()
            : EditorWindow("Preview")
            , m_currentRenderer(nullptr)
        {
        }
        
        void Render(GameContext& context) override;
        
        // Set preview renderer
        void SetPreviewRenderer(std::unique_ptr<IPreviewRenderer> renderer)
        {
            m_currentRenderer = std::move(renderer);
        }
        
        // Clear preview
        void ClearPreview()
        {
            m_currentRenderer.reset();
        }
        
        // Check if there's an active preview
        bool HasPreview() const { return m_currentRenderer != nullptr; }
        
    private:
        std::unique_ptr<IPreviewRenderer> m_currentRenderer;
    };
}
