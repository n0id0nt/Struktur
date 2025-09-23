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

// VHS/Glitch parameters
uniform float scanlineIntensity;
uniform float chromaticAberration;
uniform float glitchFrequency;
uniform float holographicShift;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    
    // Simple scan lines
    float scanlines = sin(uv.y * resolution.y) * scanlineIntensity;
    scanlines = scanlines * 0.5 + (1.0 - scanlineIntensity * 0.5);
    
    // Chromatic aberration - separate RGB channels
    float aberration = chromaticAberration * sin(time * rippleSpeed + uv.y * rippleFrequency);
    vec2 redOffset = vec2(aberration, 0.0);
    vec2 blueOffset = vec2(-aberration, 0.0);
    
    float r = texture(texture0, uv + redOffset).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv + blueOffset).b;
    float alpha = texture(texture0, uv).a;
    
    vec3 baseColor = vec3(r, g, b);
    
    // Simple glitch effect
    float glitch = step(1.0 - glitchFrequency, sin(time * 10.0 + uv.y * 20.0));
    baseColor = mix(baseColor, baseColor * soulColor, glitch * 0.5);
    
    // Holographic color shift
    float holo = sin(time * 2.0 + uv.y * holographicShift) * 0.5 + 0.5;
    vec3 holoColor = mix(vec3(0.5, 1.0, 1.0), vec3(1.0, 0.5, 1.0), holo);
    baseColor = mix(baseColor, baseColor * holoColor, 0.3);
    
    // Apply scan lines
    baseColor *= scanlines;
    
    // Simple glow effect
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center);
    float glow = (1.0 - smoothstep(0.0, 0.8, dist)) * glowIntensity;
    baseColor += soulColor * glow * 0.2;
    
    finalColor = vec4(baseColor, alpha) * colDiffuse * fragColor;
}