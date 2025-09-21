#pragma once

#include "raylib.h"

namespace Struktur
{
	namespace Component
	{
        struct Shader
        {
            ::Shader shader;
            // Soul effect -- TEMP UNTIL I HAVE BETTER SHADER HANDLING
            ::Vector3 color;        // RGB soul color
            float glowIntensity;  // Brightness (0.0-2.0)
            float rippleSpeed;    // Animation speed
            float rippleFreq;     // Ripple frequency
            float amplitude;     // How far the waves displace vertices
            float frequency;     // How many waves per unit
            float speed;         // Speed of wave animation
            ::Vector2 direction;   // Primary wave direction
        };
    }
}
