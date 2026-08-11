#pragma once

#include "PreviewRenderer.h"
#include "Engine/Util/Curve.h"
#include <glm/glm.hpp>

namespace Struktur::Debug
{
    class CurvePreviewRenderer : public IPreviewRenderer
    {
    public:
        CurvePreviewRenderer(Util::Curve* curve, const std::string& name)
            : m_curve(curve)
            , m_name(name)
            , m_selectedKeyframe(-1)
            , m_isDragging(false)
        {
            // Initialize view bounds
            curve->GetTimeRange(m_viewMinTime, m_viewMaxTime);
            curve->GetValueRange(m_viewMinValue, m_viewMaxValue);
            
            // Add padding
            float timePadding = (m_viewMaxTime - m_viewMinTime) * 0.1f;
            float valuePadding = (m_viewMaxValue - m_viewMinValue) * 0.1f;
            m_viewMinTime -= timePadding;
            m_viewMaxTime += timePadding;
            m_viewMinValue -= valuePadding;
            m_viewMaxValue += valuePadding;
        }
        
        void Render(GameContext& context, const ImVec2& availableSize) override;
        void RenderControls(GameContext& context) override;
        std::string GetPreviewName() const override { return m_name; }
        
    private:
        void RenderCurveCanvas(const ImVec2& canvasSize);
        glm::vec2 CurveToScreen(float time, float value, const ImVec2& canvasPos, const ImVec2& canvasSize);
        void ScreenToCurve(const ImVec2& screenPos, const ImVec2& canvasPos, const ImVec2& canvasSize, float& time, float& value);
        
    private:
        Util::Curve* m_curve;
        std::string m_name;
        int m_selectedKeyframe;
        bool m_isDragging;
        float m_viewMinTime, m_viewMaxTime;
        float m_viewMinValue, m_viewMaxValue;
    };
}

// Implementation (add the curve editor code from PreviewWindow here)
// ... (Same implementation as before)