#version 330 core

in vec3 Position;
in vec3 Normal;
in vec3 Color;
in vec2 UV;

uniform vec3 LightDirection;
uniform float LightIntensity;

uniform float Time;

uniform sampler2D EarthTexture;
uniform sampler2D CloudsTexture;

uniform vec2 CloudsRotationSpeed = vec2(0.008, 0.00);

out vec4 OutColor;

void main()
{
    vec3 N = normalize(Normal);

    // inverte a direção para calcular o Lambertiano
    vec3 L = -normalize(LightDirection);

    // Dot entre dois vetores unitários é equivalente ao cosseno entre esses vetores
    // Quanto maior o ângulo entre os vetores, menor é o cosseno entre eles
    float Lambertian = dot(N, L);

    // O clamp vai manter o valor do Lambertiano entre 0 e 1
    // Se o valor for negativa isso quer dizer que estamos virados pro lado
    // oposto a direção da luz.
    Lambertian = clamp(Lambertian, 0.0, 1.0);

    float SpecularReflection = 0.0;
    if (Lambertian > 0.0)
    {
        // Vetor V
        vec3 ViewDirection = -normalize(Position);

        // Vetor R
        vec3 ReflectionDirection = reflect(-L, N);

        // Termo especular: (R . V) ^ alpha
        SpecularReflection = pow(dot(ReflectionDirection, ViewDirection), 50.0);

        // Limita o valor da reflecção especular a números positivos
        SpecularReflection = max(0.0, SpecularReflection);
    }

    vec3 EarthColor = texture(EarthTexture, UV).rgb;
    vec3 CloudsColor = texture(CloudsTexture, UV + Time * CloudsRotationSpeed).rgb;

    vec3 SurfaceColor = EarthColor + CloudsColor;

    // A reflecção difusa vai ser o produto do lambertiano com a intensidade
    // da luz e a cor da textura.
    vec3 DiffuseReflection = Lambertian * LightIntensity * SurfaceColor + SpecularReflection;

    OutColor = vec4(DiffuseReflection, 1.0);
}