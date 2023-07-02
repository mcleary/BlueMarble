#version 330 core

in vec3 Position;
in vec3 Normal;
in vec2 UV;

uniform float Time;

uniform sampler2D EarthTexture;
uniform sampler2D CloudsTexture;

uniform vec2 CloudsRotationSpeed = vec2(0.008, 0.00);

out vec4 OutColor;

void main()
{
    vec3 EarthColor = texture(EarthTexture, UV).rgb;
    vec3 CloudsColor = texture(CloudsTexture, UV + Time * CloudsRotationSpeed).rgb;
    vec3 SurfaceColor = EarthColor + CloudsColor;

    OutColor = vec4(SurfaceColor, 1.0);
}
