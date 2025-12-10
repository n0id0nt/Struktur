#pragma once

#include "PreviewRenderer.h"
#include <raylib.h>

namespace Struktur
{
    namespace Component
    {
        struct Shader;
    }
}

namespace Struktur::Debug
{
    enum class ShaderPreviewShape
    {
        Quad2D,      // 2D texture on quad
        Cube,        // Cubemap
        Sphere       // Texture on sphere
    };
    
    class ShaderPreviewRenderer : public IPreviewRenderer
    {
    public:
        ShaderPreviewRenderer(const Component::Shader* shader, const std::string& name)
            : m_shader(shader)
            , m_name(name)
            , m_previewShape(ShaderPreviewShape::Quad2D)
            , m_previewTexture({0})
            , m_renderTexture({0})
            , m_useCustomTexture(false)
            , m_rotation(0.0f)
        {
            InitialisePreview();
        }
        
        ~ShaderPreviewRenderer()
        {
            CleanupPreview();
        }
        
        void Render(GameContext& context, const ImVec2& availableSize) override;
        void RenderControls(GameContext& context) override;
        std::string GetPreviewName() const override { return m_name; }
        void Clear() override { CleanupPreview(); }
        
    private:
        void InitialisePreview();
        void CleanupPreview();
        void RenderShaderPreview(const ImVec2& size);
        void ApplyShaderUniforms();
        
    private:
        const Component::Shader* m_shader;
        std::string m_name;
        ShaderPreviewShape m_previewShape;
        ::Texture2D m_previewTexture;  // Default texture
        ::RenderTexture2D m_renderTexture;
        bool m_useCustomTexture;
        float m_rotation;
    };
}
