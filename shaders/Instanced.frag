#version 330 core

in VertexData
{
    vec3 Position;
    vec3 Normal;
    vec2 UV;
} In;

layout (std140) uniform FrameUBO
{
    mat4 View;
    mat4 Projection;
    float Time;
};

uniform sampler2D EarthTexture;
uniform sampler2D CloudsTexture;

uniform vec2 CloudsRotationSpeed = vec2(0.008, 0.00);

out vec4 OutColor;

void main()
{
    vec3 EarthColor = texture(EarthTexture, In.UV).rgb;
    vec3 CloudsColor = texture(CloudsTexture, In.UV + Time * CloudsRotationSpeed).rgb;
    vec3 SurfaceColor = EarthColor + CloudsColor;

    OutColor = vec4(SurfaceColor, 1.0);
}
