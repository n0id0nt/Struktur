#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform float time;
uniform float waveAmplitude;
uniform float waveFrequency;
uniform float waveSpeed;
uniform vec2 waveDirection;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    // Copy the input texture coordinates
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Calculate wave offset based on position and time
    vec2 worldPos = vertexPosition.xy;
    
    // Create multiple wave layers for more complex motion
    float wave1 = sin(dot(worldPos, waveDirection) * waveFrequency + time * waveSpeed) * waveAmplitude;
    float wave2 = sin(dot(worldPos, vec2(-waveDirection.y, waveDirection.x)) * waveFrequency * 1.3 + time * waveSpeed * 0.7) * waveAmplitude * 0.5;
    float wave3 = sin(length(worldPos - vec2(0.5, 0.5)) * waveFrequency * 2.0 - time * waveSpeed * 1.5) * waveAmplitude * 0.3;
    
    // Combine waves for complex motion
    float totalWave = wave1 + wave2 + wave3;
    
    // Apply wave displacement to vertex position
    vec3 displacedPosition = vertexPosition;
    displacedPosition.x += totalWave * 0.5;
    displacedPosition.y += totalWave * 0.3;
    
    // Transform vertex position to screen space
    gl_Position = mvp * vec4(displacedPosition, 1.0);
}