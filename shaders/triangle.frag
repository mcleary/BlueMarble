#version 330 core

in VertexData
{
    vec3 Position;
    vec3 Normal;
    vec2 UV;
} In;

struct Light
{
    vec3 Position;
    float Intensity;
};

layout (std140) uniform LightUBO
{
    Light PointLight;
};

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
    vec3 N = normalize(In.Normal);
    vec3 L = normalize(PointLight.Position - In.Position);

    float Lambertian = max(dot(N, L), 0.0);

    vec3 EarthColor = texture(EarthTexture, In.UV).rgb;
    vec3 CloudsColor = texture(CloudsTexture, In.UV + Time * CloudsRotationSpeed).rgb;

    vec3 SurfaceColor = EarthColor + CloudsColor;

    vec3 DiffuseReflection = Lambertian * SurfaceColor * PointLight.Intensity;

    OutColor = vec4(DiffuseReflection, 1.0);
}