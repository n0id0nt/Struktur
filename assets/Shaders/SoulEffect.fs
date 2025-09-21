#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;
uniform vec2 resolution;
uniform vec3 soulColor;
uniform float glowIntensity;
uniform float rippleSpeed;
uniform float rippleFrequency;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Sample the original texture
    vec4 texelColor = texture(texture0, fragTexCoord);
    
    // Calculate distance from center for radial effects
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(fragTexCoord, center);
    
    // Create ripple effect
    float ripple = sin(dist * rippleFrequency - time * rippleSpeed) * 0.5 + 0.5;
    ripple *= (1.0 - dist); // Fade ripples toward edges
    
    // Create pulsing glow
    float pulse = sin(time * 3.0) * 0.3 + 0.7; // Oscillate between 0.4 and 1.0
    
    // Calculate glow based on distance from edges
    float glow = 1.0 - smoothstep(0.3, 0.8, dist);
    glow *= glowIntensity * pulse;
    
    // Create ethereal transparency effect
    float alpha = texelColor.a;
    if (alpha > 0.1) {
        alpha = mix(alpha, alpha * (0.7 + ripple * 0.3), glow);
    }
    
    // Combine original color with soul color
    vec3 finalRGB = mix(texelColor.rgb, soulColor, glow * 0.5);
    
    // Add rim lighting effect
    float rim = 1.0 - dot(normalize(fragTexCoord - center), normalize(fragTexCoord - center));
    rim = pow(rim, 2.0);
    finalRGB += soulColor * rim * glowIntensity * pulse;
    
    // Apply ripple distortion to the final color
    finalRGB += soulColor * ripple * 0.2;
    
    finalColor = vec4(finalRGB, alpha) * colDiffuse * fragColor;
}