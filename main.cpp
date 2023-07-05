#include <array>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Camera.h"


enum class SceneType
{
    BlueMarble,
    Cylinder,
    Ortho
};

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 UV;
};

struct Triangle
{
    GLuint V0;
    GLuint V1;
    GLuint V2;
};

struct LineVertex
{
    glm::vec3 Position;
    glm::vec3 Color;
};

struct Light
{
    glm::vec3 Position;
    float Intensity;
};

struct Geometry
{
    std::vector<Vertex> Vertices;
    std::vector<Triangle> Indices;
};

struct RenderData
{
    GLuint VAO;
    GLuint NumElements;
    glm::mat4 Transform = glm::identity<glm::mat4>();
};

struct InstancedRenderData : public RenderData
{
    GLuint NumInstances;
};

// Configurações Iniciais
GLFWwindow* Window = nullptr;
std::int32_t WindowWidth = 1920;
std::int32_t WindowHeight = 1080;
SceneType Scene = SceneType::BlueMarble;
SimpleCamera Camera;
constexpr GLuint SphereResolution = 100;
constexpr GLuint NumInstances = 10000;
Light PointLight;
bool bShowWireframe = false;
bool bCullFace = false;

Geometry GenerateSphere(GLuint InResolution)
{
    Geometry SphereGeometry;

    constexpr float Pi = glm::pi<float>();
    constexpr float TwoPi = glm::two_pi<float>();
    const float InvResolution = 1.0f / static_cast<float>(InResolution - 1);

    for (GLuint UIndex = 0; UIndex < InResolution; ++UIndex)
    {
        const float U = UIndex * InvResolution;
        const float Phi = glm::mix(0.0f, TwoPi, U);

        for (GLuint VIndex = 0; VIndex < InResolution; ++VIndex)
        {
            const float V = VIndex * InvResolution;
            const float Theta = glm::mix(0.0f, Pi, V);

            // Equação paramétrica da esfera usando o Y como eixo polar
            const glm::vec3 VertexPosition =
            {
                glm::sin(Theta) * glm::sin(Phi),
                glm::cos(Theta),
                glm::sin(Theta) * glm::cos(Phi)
            };

            const glm::vec3 VertexNormal = glm::normalize(VertexPosition);
            SphereGeometry.Vertices.emplace_back(Vertex{ .Position = VertexPosition, .Normal = VertexNormal, .UV = { U, V } });
        }
    }

    for (GLuint U = 0; U < InResolution - 1; ++U)
    {
        for (GLuint V = 0; V < InResolution - 1; ++V)
        {
            const GLuint P0 = U + V * InResolution;
            const GLuint P1 = U + 1 + V * InResolution;
            const GLuint P2 = U + (V + 1) * InResolution;
            const GLuint P3 = U + 1 + (V + 1) * InResolution;

            SphereGeometry.Indices.emplace_back(Triangle{ P3, P2, P0 });
            SphereGeometry.Indices.emplace_back(Triangle{ P1, P3, P0 });
        }
    }

    return SphereGeometry;
}

Geometry GenerateCylinder(GLuint InResolution)
{
    constexpr float CylinderHeight = 1.0f;
    constexpr float HalfCylinderHeight = CylinderHeight / 2.0f;

    Geometry CylinderGeometry;

    constexpr float TwoPi = glm::two_pi<float>();
    const float InvResolution = 1.0f / static_cast<float>(InResolution - 1);

    for (GLuint UIndex = 0; UIndex < InResolution; ++UIndex)
    {
        const float U = UIndex * InvResolution;
        const float Theta = glm::mix(0.0f, TwoPi, U);

        for (GLuint VIndex = 0; VIndex < InResolution; ++VIndex)
        {
            const float V = VIndex * InvResolution;
            const float Height = glm::mix(-HalfCylinderHeight, HalfCylinderHeight, V);

            glm::vec3 VertexPosition =
            {
                glm::sin(Theta),
                Height,
                glm::cos(Theta),
            };

            const glm::vec3 VertexNormal = glm::normalize(glm::vec3{ VertexPosition.x, 0.0f, VertexPosition.z });
            CylinderGeometry.Vertices.emplace_back(Vertex{ .Position = VertexPosition, .Normal = VertexNormal, .UV = { U, 1.0 - V } });
        }
    }

    for (GLuint U = 0; U < InResolution - 1; ++U)
    {
        for (GLuint V = 0; V < InResolution - 1; ++V)
        {
            const GLuint P0 = U + V * InResolution;
            const GLuint P1 = U + 1 + V * InResolution;
            const GLuint P2 = U + (V + 1) * InResolution;
            const GLuint P3 = U + 1 + (V + 1) * InResolution;

            CylinderGeometry.Indices.emplace_back(Triangle{ P0, P2, P3 });
            CylinderGeometry.Indices.emplace_back(Triangle{ P0, P3, P1 });
        }
    }

    const glm::vec3 TopVertex = { 0.0f, HalfCylinderHeight, 0.0f };
    const glm::vec3 BotVertex = { 0.0f, -HalfCylinderHeight, 0.0f };

    CylinderGeometry.Vertices.emplace_back(Vertex{ .Position = TopVertex, .Normal = { 0.0f, 1.0f, 0.0f }, .UV = { 1.0f, 1.0f } });
    const GLuint TopVertexIndex = static_cast<GLuint>(CylinderGeometry.Vertices.size() - 1);
    CylinderGeometry.Vertices.emplace_back(Vertex{ .Position = BotVertex, .Normal = { 0.0f, -1.0f, 0.0f }, .UV = { 0.0f, 0.0f } });
    const GLuint BotVertexIndex = static_cast<GLuint>(CylinderGeometry.Vertices.size() - 1);

    for (GLuint UIndex = 0; UIndex < InResolution - 1; ++UIndex)
    {
        GLuint U = UIndex * InResolution;

        GLuint P0 = U;
        GLuint P1 = BotVertexIndex;
        GLuint P2 = U + InResolution;

        CylinderGeometry.Indices.emplace_back(Triangle{ P0, P1, P2 });
    }

    for (GLuint UIndex = 0; UIndex < InResolution - 1; ++UIndex)
    {
        GLuint U = UIndex * InResolution + (InResolution - 1);

        GLuint P0 = U;
        GLuint P1 = (UIndex + 1) * InResolution + (InResolution - 1);
        GLuint P2 = TopVertexIndex;

        CylinderGeometry.Indices.emplace_back(Triangle{ P0, P1, P2 });
    }

    return CylinderGeometry;
}

Geometry GenerateQuad()
{
    Geometry QuadGeometry;

    constexpr glm::vec3 Normal = { 0.0f, 0.0f, 1.0f };
    QuadGeometry.Vertices =
    {
        Vertex{ .Position = { 0.0f, 0.0f, 0.0f }, .Normal = Normal, .UV = { 0.0f, 1.0f } },
        Vertex{ .Position = { 1.0f, 0.0f, 0.0f }, .Normal = Normal, .UV = { 1.0f, 1.0f } },
        Vertex{ .Position = { 1.0f, 1.0f, 0.0f }, .Normal = Normal, .UV = { 1.0f, 0.0f } },
        Vertex{ .Position = { 0.0f, 1.0f, 0.0f }, .Normal = Normal, .UV = { 0.0f, 0.0f } },
    };
    QuadGeometry.Indices =
    {
        Triangle{ 0, 1, 2 },
        Triangle{ 2, 3, 0 },
    };

    return QuadGeometry;
}

std::vector<glm::mat4> GenerateInstances(GLuint InNumInstances)
{
    std::vector<glm::mat4> ModelMatrices;
    ModelMatrices.reserve(InNumInstances);

    std::random_device Device;
    std::default_random_engine Generator(Device());

    constexpr float Jitter = 0.1f;
    std::normal_distribution<> JitterDistribution{ -Jitter, Jitter };
    std::normal_distribution<> NormalDistribution(0.0f, 0.1f);

    for (std::uint32_t Index = 0; Index < InNumInstances; ++Index)
    {
        const float Radius = 5.0f + JitterDistribution(Generator);

        const float Alpha = static_cast<float>(Index) / static_cast<float>(InNumInstances);
        const float Angle = glm::mix(0.0f, glm::two_pi<float>(), Alpha);

        const float X = Radius * glm::sin(Angle);
        const float Y = NormalDistribution(Generator);
        const float Z = Radius * glm::cos(Angle);

        glm::mat4 InstanceMatrix = glm::translate(glm::identity<glm::mat4>(), { X, Y, Z });
        constexpr float Scale = 0.01f;
        InstanceMatrix = glm::scale(InstanceMatrix, { Scale, Scale, Scale });

        ModelMatrices.emplace_back(InstanceMatrix);
    }

    std::shuffle(ModelMatrices.begin(), ModelMatrices.end(), Generator);

    return ModelMatrices;
}

std::string ReadFile(const char* FilePath)
{
    std::string FileContents;
    if (std::ifstream FileStream{ FilePath, std::ios::in })
    {
        FileContents.assign((std::istreambuf_iterator<char>(FileStream)), std::istreambuf_iterator<char>());
    }
    return FileContents;
}

void CheckShader(GLuint ShaderId)
{
    // Verificar se o shader foi compilado
    GLint Result = GL_TRUE;
    glGetShaderiv(ShaderId, GL_COMPILE_STATUS, &Result);

    if (Result == GL_FALSE)
    {
        // Erro ao compilar o shader, imprimir o log para saber o que está errado
        GLint InfoLogLength = 0;
        glGetShaderiv(ShaderId, GL_INFO_LOG_LENGTH, &InfoLogLength);

        if (InfoLogLength > 0)
        {
            GLint ShaderType = 0;
            glGetShaderiv(ShaderId, GL_SHADER_TYPE, &ShaderType);

            std::string ShaderInfoLog(InfoLogLength, '\0');
            glGetShaderInfoLog(ShaderId, InfoLogLength, nullptr, &ShaderInfoLog[0]);

            std::string_view ShaderTypeStr;
            switch (ShaderType)
            {
                case GL_VERTEX_SHADER:
                    ShaderTypeStr = "Vertex";
                    break;

                case GL_FRAGMENT_SHADER:
                    ShaderTypeStr = "Fragment";
                    break;

                default:
                    assert(false);
            }

            std::cout << "Erro no " << ShaderTypeStr << " Shader: " << std::endl;
            std::cout << ShaderInfoLog << std::endl;

            assert(false);
        }
    }
}

GLuint LoadShaders(const char* VertexShaderFile, const char* FragmentShaderFile)
{
    // Criar os identificadores de cada um dos shaders
    GLuint VertShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    std::string VertexShaderSource = ReadFile(VertexShaderFile);
    std::string FragmentShaderSource = ReadFile(FragmentShaderFile);

    assert(!VertexShaderSource.empty());
    assert(!FragmentShaderSource.empty());

    std::cout << "Compilando " << VertexShaderFile << std::endl;
    const char* VertexShaderSourcePtr = VertexShaderSource.c_str();
    glShaderSource(VertShaderId, 1, &VertexShaderSourcePtr, nullptr);
    glCompileShader(VertShaderId);
    CheckShader(VertShaderId);

    std::cout << "Compilando " << FragmentShaderFile << std::endl;
    const char* FragmentShaderSourcePtr = FragmentShaderSource.c_str();
    glShaderSource(FragShaderId, 1, &FragmentShaderSourcePtr, nullptr);
    glCompileShader(FragShaderId);
    CheckShader(FragShaderId);

    std::cout << "Linkando Programa" << std::endl;
    GLuint ProgramId = glCreateProgram();
    glAttachShader(ProgramId, VertShaderId);
    glAttachShader(ProgramId, FragShaderId);
    glLinkProgram(ProgramId);

    // Verificar o programa
    GLint Result = GL_TRUE;
    glGetProgramiv(ProgramId, GL_LINK_STATUS, &Result);

    if (Result == GL_FALSE)
    {
        GLint InfoLogLength = 0;
        glGetProgramiv(ProgramId, GL_INFO_LOG_LENGTH, &InfoLogLength);

        if (InfoLogLength > 0)
        {
            std::string ProgramInfoLog(InfoLogLength, '\0');
            glGetProgramInfoLog(ProgramId, InfoLogLength, nullptr, &ProgramInfoLog[0]);

            std::cout << "Erro ao linkar programa" << std::endl;
            std::cout << ProgramInfoLog << std::endl;

            assert(false);
        }
    }

    glDetachShader(ProgramId, VertShaderId);
    glDetachShader(ProgramId, FragShaderId);

    glDeleteShader(VertShaderId);
    glDeleteShader(FragShaderId);

    return ProgramId;
}

GLuint LoadTexture(const char* TextureFile)
{
    std::cout << "Carregando Textura " << TextureFile << std::endl;

    int TextureWidth = 0;
    int TextureHeight = 0;
    int NumberOfComponents = 0;
    unsigned char* TextureData = stbi_load(TextureFile, &TextureWidth, &TextureHeight, &NumberOfComponents, 3);
    assert(TextureData);

    // Gerar o Identifador da Textura
    GLuint TextureId;
    glGenTextures(1, &TextureId);

    // Habilita a textura para ser modificada
    glBindTexture(GL_TEXTURE_2D, TextureId);

    // Copia a textura para a memória da GPU
    GLint Level = 0;
    GLint Border = 0;
    glTexImage2D(GL_TEXTURE_2D, Level, GL_RGB, TextureWidth, TextureHeight, Border, GL_RGB, GL_UNSIGNED_BYTE, TextureData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(TextureData);
    return TextureId;
}

void MouseButtonCallback(GLFWwindow* Window, std::int32_t Button, std::int32_t Action, std::int32_t Modifiers)
{
    if (Button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (Action == GLFW_PRESS)
        {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            double X, Y;
            glfwGetCursorPos(Window, &X, &Y);

            Camera.PreviousCursor = glm::vec2{ X, Y };
            Camera.bEnableMouseMovement = true;
        }
        else if (Action == GLFW_RELEASE)
        {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            Camera.bEnableMouseMovement = false;
        }
    }
}

void MouseMotionCallback(GLFWwindow* Window, double X, double Y)
{
    Camera.MouseMove(static_cast<float>(X), static_cast<float>(Y));

    if (Scene == SceneType::Ortho)
    {
        PointLight.Position.x = static_cast<float>(X) / WindowWidth;
        PointLight.Position.y = static_cast<float>(WindowHeight - Y) / WindowHeight;
    }
}

void KeyCallback(GLFWwindow* Window, std::int32_t Key, std::int32_t ScanCode, std::int32_t Action, std::int32_t Modifers)
{
    if (Action == GLFW_PRESS)
    {
        switch (Key)
        {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(Window, true);
                break;

            case GLFW_KEY_W:
                Camera.MoveForward(1.0f);
                break;

            case GLFW_KEY_S:
                Camera.MoveForward(-1.0f);
                break;

            case GLFW_KEY_A:
                Camera.MoveRight(-1.0f);
                break;

            case GLFW_KEY_D:
                Camera.MoveRight(1.0f);
                break;

            default:
                break;
        }
    }
    else if (Action == GLFW_RELEASE)
    {
        switch (Key)
        {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(Window, true);
                break;

            case GLFW_KEY_W:
                Camera.MoveForward(0.0f);
                break;

            case GLFW_KEY_S:
                Camera.MoveForward(0.0f);
                break;

            case GLFW_KEY_A:
                Camera.MoveRight(0.0f);
                break;

            case GLFW_KEY_D:
                Camera.MoveRight(0.0f);
                break;

            case GLFW_KEY_O:
                Camera.bIsOrtho = !Camera.bIsOrtho;
                break;

            case GLFW_KEY_R:
                Camera.Reset();
                break;

            case GLFW_KEY_M:
                bShowWireframe = !bShowWireframe;
                break;

            case GLFW_KEY_N:
                bCullFace = !bCullFace;
                break;

            default:
                break;
        }
    }
}

void ResizeCallback(GLFWwindow* Window, std::int32_t Width, std::int32_t Height)
{
    WindowWidth = Width;
    WindowHeight = Height;

    Camera.SetViewportSize(Width, Height);

    glViewport(0, 0, Width, Height);
}

RenderData GetRenderData()
{
    Geometry Geo;
    RenderData GeoRenderData;

    switch (Scene)
    {
        case SceneType::BlueMarble:
            Geo = GenerateSphere(SphereResolution);
            GeoRenderData.Transform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(180.0f), glm::vec3{ 0.0f, 1.0f, 0.0f });
            Camera.bIsOrtho = false;
            PointLight.Position = glm::vec3(0.0f, 0.0f, 1000.0f);
            PointLight.Intensity = 1.0f;
            break;

        case SceneType::Ortho:
            Geo = GenerateQuad();
            Camera.bIsOrtho = true;
            PointLight.Position = glm::vec3(0.0f, 0.0f, 0.05f);
            PointLight.Intensity = 1.0f;
            break;

        case SceneType::Cylinder:
            Geo = GenerateCylinder(20);
            Camera.bIsOrtho = false;
            PointLight.Position = glm::vec3(0.0f, 0.0f, 1000.0f);
            PointLight.Intensity = 1.0f;
            break;

        default:
            exit(1);
    }

    GLuint VertexBuffer, ElementBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Geo.Vertices.size() * sizeof(Vertex), Geo.Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ElementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Geo.Indices.size() * sizeof(Triangle), Geo.Indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GeoRenderData.NumElements = (GLuint) Geo.Indices.size() * 3;

    glGenVertexArrays(1, &GeoRenderData.VAO);
    glBindVertexArray(GeoRenderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);

    // Gerar o identificador do VAO
    // Identificador do Vertex Array Object (VAO)
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Informa ao OpenGL onde, dentro do VertexBuffer, os vértices estão. No
    // nosso caso o array Triangles é tudo o que a gente precisa
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, UV)));

    // Disabilitar o VAO
    glBindVertexArray(0);

    return GeoRenderData;
}

RenderData GetAxisRenderData()
{
    const std::array<LineVertex, 6> Vertices =
    {
        LineVertex{ .Position = { 0.0f, 0.0f, 0.0f }, .Color = { 1.0f, 0.0f, 0.0f } },
        LineVertex{ .Position = { 10.0f, 0.0f, 0.0f }, .Color = { 1.0f, 0.0f, 0.0f } },

        LineVertex{ .Position = { 0.0f, 0.0f, 0.0f }, .Color = { 0.0f, 1.0f, 0.0f } },
        LineVertex{ .Position = { 0.0f, 10.0f, 0.0f }, .Color = { 0.0f, 1.0f, 0.0f } },

        LineVertex{ .Position = { 0.0f, 0.0f, 0.0f }, .Color = { 0.0f, 0.0f, 1.0f } },
        LineVertex{ .Position = { 0.0f, 0.0f, 10.0f }, .Color = { 0.0f, 0.0f, 1.0f } },
    };

    GLuint VertexBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertex), Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    RenderData AxisRenderData;
    AxisRenderData.NumElements = (GLuint) Vertices.size();

    glGenVertexArrays(1, &AxisRenderData.VAO);
    glBindVertexArray(AxisRenderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, Color)));

    glBindVertexArray(0);

    return AxisRenderData;
}

InstancedRenderData GetInstancedRenderData()
{
    Geometry Geo = GenerateSphere(10);
    std::vector<glm::mat4> Instances = GenerateInstances(NumInstances);

    GLuint VertexBuffer, ElementBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Geo.Vertices.size() * sizeof(Vertex), Geo.Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ElementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Geo.Indices.size() * sizeof(Triangle), Geo.Indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLuint InstancesBuffer;
    glGenBuffers(1, &InstancesBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, InstancesBuffer);
    glBufferData(GL_ARRAY_BUFFER, Instances.size() * sizeof(glm::mat4), &Instances[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint InstanceVAO;
    glGenVertexArrays(1, &InstanceVAO);
    glBindVertexArray(InstanceVAO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);

    // Faz o bind do VertexBuffer e configura o layout dos atributos 0, 1 e 2
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, UV)));

    // configura o array de matrizes nos atributos 3, 4, 5, 6
    glBindBuffer(GL_ARRAY_BUFFER, InstancesBuffer);

    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);

    InstancedRenderData InstRenderData;
    InstRenderData.VAO = InstanceVAO;
    InstRenderData.NumInstances = static_cast<GLuint>(Instances.size());
    InstRenderData.NumElements = static_cast<GLuint>(Geo.Indices.size()) * 3;
    return InstRenderData;
}

int main()
{
    if (!glfwInit())
    {
        std::cout << "Erro ao inicializar o GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_DEPTH_BITS, 32);

    Window = glfwCreateWindow(WindowWidth, WindowHeight, "BlueMarble", nullptr, nullptr);
    if (!Window)
    {
        std::cout << "Erro ao criar janela" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetFramebufferSizeCallback(Window, ResizeCallback);
    glfwSetMouseButtonCallback(Window, MouseButtonCallback);
    glfwSetCursorPosCallback(Window, MouseMotionCallback);
    glfwSetKeyCallback(Window, KeyCallback);

    glfwMakeContextCurrent(Window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Erro ao inicializar o GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Imprime informações sobre a versão do OpenGL que o programa está usando
    GLint GLMajorVersion = 0;
    GLint GLMinorVersion = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &GLMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &GLMinorVersion);
    std::cout << "OpenGL Version  : " << GLMajorVersion << "." << GLMinorVersion << std::endl;
    std::cout << "OpenGL Vendor   : " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer : " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Version  : " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version    : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Compilar o vertex e o fragment shader
    GLuint ProgramId = LoadShaders("shaders/triangle.vert", "shaders/triangle.frag");
    GLuint InstancedProgramId = LoadShaders("shaders/instanced.vert", "shaders/instanced.frag");
    GLuint AxisProgramId = LoadShaders("shaders/lines.vert", "shaders/lines.frag");

    RenderData AxisRenderData = GetAxisRenderData();
    RenderData GeoRenderData = GetRenderData();
    InstancedRenderData InstRenderData = GetInstancedRenderData();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Carregar a Textura para a Memória de Vídeo
    GLuint EarthTextureId = LoadTexture("textures/earth_2k.jpg");
    GLuint CloudsTextureId = LoadTexture("textures/earth_clouds_2k.jpg");

    // Configura a cor de fundo
    glClearColor(0.1f, 0.1f, 0.1f, 1.0);

    Camera.SetViewportSize(WindowWidth, WindowHeight);

    double PreviousTime = glfwGetTime();

    while (!glfwWindowShouldClose(Window))
    {
        if (bCullFace)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        const double CurrentTime = glfwGetTime();
        const double DeltaTime = CurrentTime - PreviousTime;
        if (DeltaTime > 0.0)
        {
            Camera.Update(static_cast<float>(DeltaTime));
            PreviousTime = CurrentTime;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 ViewMatrix = Camera.GetView();
        const glm::mat4 ViewProjection = Camera.GetViewProjection();

        {
            glUseProgram(AxisProgramId);

            const glm::mat4 ModelViewProjectionMatrix = ViewProjection;

            GLint ModelViewProjectionLoc = glGetUniformLocation(AxisProgramId, "ModelViewProjection");
            glUniformMatrix4fv(ModelViewProjectionLoc, 1, GL_FALSE, glm::value_ptr(ModelViewProjectionMatrix));

            glBindVertexArray(AxisRenderData.VAO);
            glDrawArrays(GL_LINES, 0, AxisRenderData.NumElements);
            glBindVertexArray(0);
        }

        {
            glUseProgram(ProgramId);

            const glm::mat4 ModelMatrix = GeoRenderData.Transform;
            const glm::mat4 NormalMatrix = glm::transpose(glm::inverse(ModelMatrix));
            const glm::mat4 ModelViewProjectionMatrix = ViewProjection * ModelMatrix;

            GLint TimeLoc = glGetUniformLocation(ProgramId, "Time");
            glUniform1f(TimeLoc, static_cast<GLfloat>(CurrentTime));

            GLint NormalMatrixLoc = glGetUniformLocation(ProgramId, "NormalMatrix");
            glUniformMatrix4fv(NormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(NormalMatrix));

            GLint ModelMatrixLoc = glGetUniformLocation(ProgramId, "ModelMatrix");
            glUniformMatrix4fv(ModelMatrixLoc, 1, GL_FALSE, glm::value_ptr(ModelMatrix));

            GLint ModelViewProjectionLoc = glGetUniformLocation(ProgramId, "ModelViewProjection");
            glUniformMatrix4fv(ModelViewProjectionLoc, 1, GL_FALSE, glm::value_ptr(ModelViewProjectionMatrix));

            GLint LightIntensityLoc = glGetUniformLocation(ProgramId, "PointLight.Intensity");
            glUniform1f(LightIntensityLoc, PointLight.Intensity);

            GLint LightPositionLoc = glGetUniformLocation(ProgramId, "PointLight.Position");
            glUniform3fv(LightPositionLoc, 1, glm::value_ptr(PointLight.Position));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, EarthTextureId);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, CloudsTextureId);

            GLint TextureSamplerLoc = glGetUniformLocation(ProgramId, "EarthTexture");
            glUniform1i(TextureSamplerLoc, 0);

            GLint CloudsTextureSamplerLoc = glGetUniformLocation(ProgramId, "CloudsTexture");
            glUniform1i(CloudsTextureSamplerLoc, 1);

            glPolygonMode(GL_FRONT_AND_BACK, bShowWireframe ? GL_LINE : GL_FILL);
            glBindVertexArray(GeoRenderData.VAO);
            glDrawElements(GL_TRIANGLES, GeoRenderData.NumElements, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        if (true)
        {
            // Render Instanced Data
            glUseProgram(InstancedProgramId);

            GLuint TimeLoc = glGetUniformLocation(InstancedProgramId, "Time");
            glUniform1f(TimeLoc, static_cast<GLfloat>(CurrentTime));

            GLint NumInstancesLoc = glGetUniformLocation(InstancedProgramId, "NumInstances");
            glUniform1i(NumInstancesLoc, InstRenderData.NumInstances);

            GLint TextureSamplerLoc = glGetUniformLocation(InstancedProgramId, "EarthTexture");
            glUniform1i(TextureSamplerLoc, 0);

            GLint CloudsTextureSamplerLoc = glGetUniformLocation(InstancedProgramId, "CloudsTexture");
            glUniform1i(CloudsTextureSamplerLoc, 1);

            GLint ViewProjectionLoc = glGetUniformLocation(InstancedProgramId, "ViewProjection");
            glUniformMatrix4fv(ViewProjectionLoc, 1, GL_FALSE, glm::value_ptr(ViewProjection));

            glBindVertexArray(InstRenderData.VAO);
            glDrawElementsInstanced(GL_TRIANGLES, InstRenderData.NumElements, GL_UNSIGNED_INT, nullptr, InstRenderData.NumInstances);
            glBindVertexArray(0);
        }

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }

    glfwDestroyWindow(Window);
    glfwTerminate();

    return 0;
}
