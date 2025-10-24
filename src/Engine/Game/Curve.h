#pragma once

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

namespace Struktur::GameResource
{
    // Interpolation modes for curve segments
    enum class CurveInterpolation
    {
        Linear,      // Straight line between points
        Smooth,      // Smooth curve (Catmull-Rom spline)
        Step,        // Hold value until next keyframe
        Bezier       // Cubic bezier curve
    };
    
    // A single keyframe on the curve
    struct CurveKeyframe
    {
        float time;          // Time/X position
        float value;         // Value/Y position
        float inTangent;     // Incoming tangent (for smooth/bezier)
        float outTangent;    // Outgoing tangent (for smooth/bezier)
        CurveInterpolation interpolation;
        
        CurveKeyframe()
            : time(0.0f)
            , value(0.0f)
            , inTangent(0.0f)
            , outTangent(0.0f)
            , interpolation(CurveInterpolation::Smooth)
        {
        }
        
        CurveKeyframe(float t, float v, CurveInterpolation interp = CurveInterpolation::Smooth)
            : time(t)
            , value(v)
            , inTangent(0.0f)
            , outTangent(0.0f)
            , interpolation(interp)
        {
        }
        
        bool operator<(const CurveKeyframe& other) const
        {
            return time < other.time;
        }
    };
    
    // Animation curve class (like Unity's AnimationCurve)
    class Curve
    {
    public:
        Curve()
            : m_preWrapMode(WrapMode::Clamp)
            , m_postWrapMode(WrapMode::Clamp)
        {
            // Create default linear curve from 0 to 1
            AddKeyframe(0.0f, 0.0f);
            AddKeyframe(1.0f, 1.0f);
        }
        
        // Wrap modes for values outside the curve range
        enum class WrapMode
        {
            Clamp,      // Clamp to first/last value
            Loop,       // Loop the curve
            PingPong    // Bounce back and forth
        };
        
        // Add a keyframe to the curve
        int AddKeyframe(float time, float value, CurveInterpolation interpolation = CurveInterpolation::Smooth)
        {
            CurveKeyframe keyframe(time, value, interpolation);
            m_keyframes.push_back(keyframe);
            std::sort(m_keyframes.begin(), m_keyframes.end());
            RecalculateTangents();
            return FindKeyframeIndex(time);
        }
        
        // Remove a keyframe at index
        void RemoveKeyframe(int index)
        {
            if (index >= 0 && index < (int)m_keyframes.size())
            {
                m_keyframes.erase(m_keyframes.begin() + index);
                RecalculateTangents();
            }
        }
        
        // Move a keyframe
        void MoveKeyframe(int index, float newTime, float newValue)
        {
            if (index >= 0 && index < (int)m_keyframes.size())
            {
                m_keyframes[index].time = newTime;
                m_keyframes[index].value = newValue;
                std::sort(m_keyframes.begin(), m_keyframes.end());
                RecalculateTangents();
            }
        }
        
        // Evaluate the curve at a given time
        float Evaluate(float time) const
        {
            if (m_keyframes.empty())
                return 0.0f;
            
            if (m_keyframes.size() == 1)
                return m_keyframes[0].value;
            
            // Handle wrap modes
            float minTime = m_keyframes.front().time;
            float maxTime = m_keyframes.back().time;
            
            if (time < minTime)
            {
                switch (m_preWrapMode)
                {
                    case WrapMode::Clamp:
                        return m_keyframes.front().value;
                    case WrapMode::Loop:
                        time = maxTime - fmod(minTime - time, maxTime - minTime);
                        break;
                    case WrapMode::PingPong:
                        // TODO: Implement ping pong
                        return m_keyframes.front().value;
                }
            }
            else if (time > maxTime)
            {
                switch (m_postWrapMode)
                {
                    case WrapMode::Clamp:
                        return m_keyframes.back().value;
                    case WrapMode::Loop:
                        time = minTime + fmod(time - minTime, maxTime - minTime);
                        break;
                    case WrapMode::PingPong:
                        // TODO: Implement ping pong
                        return m_keyframes.back().value;
                }
            }
            
            // Find the two keyframes to interpolate between
            for (size_t i = 0; i < m_keyframes.size() - 1; i++)
            {
                if (time >= m_keyframes[i].time && time <= m_keyframes[i + 1].time)
                {
                    return InterpolateBetween(m_keyframes[i], m_keyframes[i + 1], time);
                }
            }
            
            return m_keyframes.back().value;
        }
        
        // Get keyframe at index
        const CurveKeyframe& GetKeyframe(int index) const
        {
            return m_keyframes[index];
        }
        
        CurveKeyframe& GetKeyframe(int index)
        {
            return m_keyframes[index];
        }
        
        // Get number of keyframes
        int GetKeyframeCount() const
        {
            return (int)m_keyframes.size();
        }
        
        // Get all keyframes
        const std::vector<CurveKeyframe>& GetKeyframes() const
        {
            return m_keyframes;
        }
        
        // Get time range
        void GetTimeRange(float& minTime, float& maxTime) const
        {
            if (m_keyframes.empty())
            {
                minTime = maxTime = 0.0f;
                return;
            }
            minTime = m_keyframes.front().time;
            maxTime = m_keyframes.back().time;
        }
        
        // Get value range
        void GetValueRange(float& minValue, float& maxValue) const
        {
            if (m_keyframes.empty())
            {
                minValue = maxValue = 0.0f;
                return;
            }
            
            minValue = maxValue = m_keyframes[0].value;
            for (const auto& kf : m_keyframes)
            {
                minValue = std::min(minValue, kf.value);
                maxValue = std::max(maxValue, kf.value);
            }
        }
        
        // Wrap modes
        void SetPreWrapMode(WrapMode mode) { m_preWrapMode = mode; }
        void SetPostWrapMode(WrapMode mode) { m_postWrapMode = mode; }
        WrapMode GetPreWrapMode() const { return m_preWrapMode; }
        WrapMode GetPostWrapMode() const { return m_postWrapMode; }
        
        // Clear all keyframes
        void Clear()
        {
            m_keyframes.clear();
        }
        
    private:
        std::vector<CurveKeyframe> m_keyframes;
        WrapMode m_preWrapMode;
        WrapMode m_postWrapMode;
        
        // Find keyframe index by time
        int FindKeyframeIndex(float time) const
        {
            for (size_t i = 0; i < m_keyframes.size(); i++)
            {
                if (std::abs(m_keyframes[i].time - time) < 0.001f)
                    return (int)i;
            }
            return -1;
        }
        
        // Interpolate between two keyframes
        float InterpolateBetween(const CurveKeyframe& a, const CurveKeyframe& b, float time) const
        {
            float t = (time - a.time) / (b.time - a.time);
            
            switch (a.interpolation)
            {
                case CurveInterpolation::Linear:
                    return glm::mix(a.value, b.value, t);
                    
                case CurveInterpolation::Step:
                    return a.value;
                    
                case CurveInterpolation::Smooth:
                {
                    // Hermite interpolation using tangents
                    float t2 = t * t;
                    float t3 = t2 * t;
                    
                    float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
                    float h2 = -2.0f * t3 + 3.0f * t2;
                    float h3 = t3 - 2.0f * t2 + t;
                    float h4 = t3 - t2;
                    
                    float duration = b.time - a.time;
                    return h1 * a.value + h2 * b.value + h3 * a.outTangent * duration + h4 * b.inTangent * duration;
                }
                    
                case CurveInterpolation::Bezier:
                {
                    // Cubic bezier
                    float t2 = t * t;
                    float t3 = t2 * t;
                    float mt = 1.0f - t;
                    float mt2 = mt * mt;
                    float mt3 = mt2 * mt;
                    
                    float duration = b.time - a.time;
                    float p1 = a.value + a.outTangent * duration / 3.0f;
                    float p2 = b.value - b.inTangent * duration / 3.0f;
                    
                    return mt3 * a.value + 3.0f * mt2 * t * p1 + 3.0f * mt * t2 * p2 + t3 * b.value;
                }
            }
            
            return a.value;
        }
        
        // Recalculate tangents for smooth curves
        void RecalculateTangents()
        {
            if (m_keyframes.size() < 2)
                return;
            
            for (size_t i = 0; i < m_keyframes.size(); i++)
            {
                if (m_keyframes[i].interpolation != CurveInterpolation::Smooth)
                    continue;
                
                // Calculate tangent based on neighboring points
                if (i == 0)
                {
                    // First point - use forward difference
                    float dt = m_keyframes[i + 1].time - m_keyframes[i].time;
                    float dv = m_keyframes[i + 1].value - m_keyframes[i].value;
                    m_keyframes[i].outTangent = dv / dt;
                    m_keyframes[i].inTangent = m_keyframes[i].outTangent;
                }
                else if (i == m_keyframes.size() - 1)
                {
                    // Last point - use backward difference
                    float dt = m_keyframes[i].time - m_keyframes[i - 1].time;
                    float dv = m_keyframes[i].value - m_keyframes[i - 1].value;
                    m_keyframes[i].inTangent = dv / dt;
                    m_keyframes[i].outTangent = m_keyframes[i].inTangent;
                }
                else
                {
                    // Middle point - use central difference
                    float dt = m_keyframes[i + 1].time - m_keyframes[i - 1].time;
                    float dv = m_keyframes[i + 1].value - m_keyframes[i - 1].value;
                    float tangent = dv / dt;
                    m_keyframes[i].inTangent = tangent;
                    m_keyframes[i].outTangent = tangent;
                }
            }
        }
    };
}