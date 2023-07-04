#version 330 core

in vec3 Position;
in vec3 Normal;
in vec2 UV;

struct Light
{
    vec3 Position;
    float Intensity;
};

uniform Light PointLight;

uniform float Time;

uniform sampler2D EarthTexture;
uniform sampler2D CloudsTexture;

uniform vec2 CloudsRotationSpeed = vec2(0.008, 0.00);

out vec4 OutColor;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(PointLight.Position - Position);

    float Lambertian = max(dot(N, L), 0.0);

    vec3 EarthColor = texture(EarthTexture, UV).rgb;
    vec3 CloudsColor = texture(CloudsTexture, UV + Time * CloudsRotationSpeed).rgb;

    vec3 SurfaceColor = EarthColor + CloudsColor;

    vec3 DiffuseReflection = Lambertian * SurfaceColor * PointLight.Intensity;

    OutColor = vec4(DiffuseReflection, 1.0);
}