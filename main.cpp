#include <array>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <random>
#include <future>
#include <thread>
#include <functional>
#include <numeric>
#include <filesystem>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Camera.h"
#include "ShaderManager.h"

enum class ESceneType
{
    BlueMarble,
    Cylinder,
    Cube,
    Ortho
};

struct FVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 UV;
};

struct FTriangle
{
    GLuint V0;
    GLuint V1;
    GLuint V2;
};

struct FLineVertex
{
    glm::vec3 Position;
    glm::vec3 Color;
};

struct FLight
{
    glm::vec3 Position;
    float Intensity;
};

struct FGeometry
{
    std::vector<FVertex> Vertices;
    std::vector<FTriangle> Indices;
};

struct FRenderData
{
    GLuint VAO;
    GLuint NumElements;
    glm::mat4 Transform = glm::identity<glm::mat4>();
};

struct FInstancedRenderData : public FRenderData
{
    GLuint NumInstances;
};

struct FUBOMatrices
{
    glm::mat4 View;
    glm::mat4 Projection;
};

struct FSimulationConfig
{
    bool bPause = true;
    bool bReverse = false;
    double TotalTime = 0.0f;
    double FramesPerSecond = 0.0f;
    double FrameTime = 0.0f;
    std::uint32_t FrameCount = 0;
    std::uint32_t TotalFrames = 0;

    std::vector<float> FrameTimeHistory;
    std::vector<float> FramesPerSecondHistory;

    std::uint32_t NumFramePlotValues = 120;
    std::uint32_t FramePlotOffset = 0;
};

struct FRenderConfig
{
    bool bShowWireframe = false;
    bool bCullFace = false;
    bool bDrawAxis = true;
    bool bDrawObject = true;
    bool bDrawInstances = true;
    bool bEnableVsync = true;

    FShaderManager ShaderManager;
};

struct FViewportConfig
{
    GLFWwindow* Window = nullptr;
    std::int32_t WindowWidth = 1920;
    std::int32_t WindowHeight = 1080;
};

struct FSceneConfig
{
    static constexpr ESceneType SceneType = ESceneType::BlueMarble;
    static constexpr GLuint SphereResolution = 100;
    std::int32_t NumInstances = 500'000;

    SimpleCamera Camera;
    FLight PointLight;
};

struct FMouse
{
    glm::ivec2 PreviousMousePos = { -1, -1 };
    glm::ivec2 MousePos;
    glm::ivec2 MouseDelta = { 0, 0 };
};

struct FInputConfig
{
    FMouse Mouse;
};

struct FConfig
{
    FSimulationConfig Simulation;
    FRenderConfig Render;
    FSceneConfig Scene;
    FViewportConfig Viewport;
    FInputConfig Input;
};

FConfig gConfig;

FGeometry GenerateSphere(GLuint InResolution)
{
    FGeometry SphereGeometry;

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
            SphereGeometry.Vertices.emplace_back(FVertex{ .Position = VertexPosition, .Normal = VertexNormal, .UV = { U, V } });
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

            SphereGeometry.Indices.emplace_back(FTriangle{ P3, P2, P0 });
            SphereGeometry.Indices.emplace_back(FTriangle{ P1, P3, P0 });
        }
    }

    return SphereGeometry;
}

FGeometry GenerateCylinder(GLuint InResolution)
{
    constexpr float CylinderHeight = 1.0f;
    constexpr float HalfCylinderHeight = CylinderHeight / 2.0f;

    FGeometry CylinderGeometry;

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
            CylinderGeometry.Vertices.emplace_back(FVertex{ .Position = VertexPosition, .Normal = VertexNormal, .UV = { U, 1.0 - V } });
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

            CylinderGeometry.Indices.emplace_back(FTriangle{ P0, P2, P3 });
            CylinderGeometry.Indices.emplace_back(FTriangle{ P0, P3, P1 });
        }
    }

    const glm::vec3 TopVertex = { 0.0f, HalfCylinderHeight, 0.0f };
    const glm::vec3 BotVertex = { 0.0f, -HalfCylinderHeight, 0.0f };

    CylinderGeometry.Vertices.emplace_back(FVertex{ .Position = TopVertex, .Normal = { 0.0f, 1.0f, 0.0f }, .UV = { 1.0f, 1.0f } });
    const GLuint TopVertexIndex = static_cast<GLuint>(CylinderGeometry.Vertices.size() - 1);
    CylinderGeometry.Vertices.emplace_back(FVertex{ .Position = BotVertex, .Normal = { 0.0f, -1.0f, 0.0f }, .UV = { 0.0f, 0.0f } });
    const GLuint BotVertexIndex = static_cast<GLuint>(CylinderGeometry.Vertices.size() - 1);

    for (GLuint UIndex = 0; UIndex < InResolution - 1; ++UIndex)
    {
        GLuint U = UIndex * InResolution;

        GLuint P0 = U;
        GLuint P1 = BotVertexIndex;
        GLuint P2 = U + InResolution;

        CylinderGeometry.Indices.emplace_back(FTriangle{ P0, P1, P2 });
    }

    for (GLuint UIndex = 0; UIndex < InResolution - 1; ++UIndex)
    {
        GLuint U = UIndex * InResolution + (InResolution - 1);

        GLuint P0 = U;
        GLuint P1 = (UIndex + 1) * InResolution + (InResolution - 1);
        GLuint P2 = TopVertexIndex;

        CylinderGeometry.Indices.emplace_back(FTriangle{ P0, P1, P2 });
    }

    return CylinderGeometry;
}

FGeometry GenerateQuad()
{
    FGeometry QuadGeometry;

    constexpr glm::vec3 Normal = { 0.0f, 0.0f, 1.0f };
    QuadGeometry.Vertices =
    {
        FVertex{.Position = { 0.0f, 0.0f, 0.0f }, .Normal = Normal, .UV = { 0.0f, 1.0f } },
        FVertex{.Position = { 1.0f, 0.0f, 0.0f }, .Normal = Normal, .UV = { 1.0f, 1.0f } },
        FVertex{.Position = { 1.0f, 1.0f, 0.0f }, .Normal = Normal, .UV = { 1.0f, 0.0f } },
        FVertex{.Position = { 0.0f, 1.0f, 0.0f }, .Normal = Normal, .UV = { 0.0f, 0.0f } },
    };
    QuadGeometry.Indices =
    {
        FTriangle{ 0, 1, 2 },
        FTriangle{ 2, 3, 0 },
    };

    return QuadGeometry;
}

FGeometry GenerateCube()
{
    FGeometry CubeGeometry;

    constexpr glm::vec3 XNormal = { 1.0f, 0.0f, 0.0f };
    constexpr glm::vec3 YNormal = { 0.0f, 1.0f, 0.0f };
    constexpr glm::vec3 ZNormal = { 0.0f, 0.0f, 1.0f };

    CubeGeometry.Vertices =
    {
        // +Z
        FVertex{ .Position = { -0.5f, -0.5f, 0.5f }, .Normal = ZNormal, . UV = { 0.0f, 1.0f } },
        FVertex{ .Position = {  0.5f, -0.5f, 0.5f }, .Normal = ZNormal, . UV = { 1.0f, 1.0f } },
        FVertex{ .Position = {  0.5f,  0.5f, 0.5f }, .Normal = ZNormal, . UV = { 1.0f, 0.0f } },
        FVertex{ .Position = { -0.5f,  0.5f, 0.5f }, .Normal = ZNormal, . UV = { 0.0f, 0.0f } },

        // -Z
        FVertex{ .Position = { -0.5f, -0.5f, -0.5f }, .Normal = -ZNormal, . UV = { 0.0f, 1.0f } },
        FVertex{ .Position = {  0.5f, -0.5f, -0.5f }, .Normal = -ZNormal, . UV = { 1.0f, 1.0f } },
        FVertex{ .Position = {  0.5f,  0.5f, -0.5f }, .Normal = -ZNormal, . UV = { 1.0f, 0.0f } },
        FVertex{ .Position = { -0.5f,  0.5f, -0.5f }, .Normal = -ZNormal, . UV = { 0.0f, 0.0f } },

        // +X
        FVertex{ .Position = { 0.5f, -0.5f, -0.5f }, .Normal = XNormal, . UV = { 0.0f, 1.0f } },
        FVertex{ .Position = { 0.5f,  0.5f, -0.5f }, .Normal = XNormal, . UV = { 1.0f, 1.0f } },
        FVertex{ .Position = { 0.5f,  0.5f,  0.5f }, .Normal = XNormal, . UV = { 1.0f, 0.0f } },
        FVertex{ .Position = { 0.5f, -0.5f,  0.5f }, .Normal = XNormal, . UV = { 0.0f, 0.0f } },

        // -X
        FVertex{ .Position = { -0.5f, -0.5f, -0.5f }, .Normal = -XNormal, . UV = { 0.0f, 1.0f } },
        FVertex{ .Position = { -0.5f,  0.5f, -0.5f }, .Normal = -XNormal, . UV = { 1.0f, 1.0f } },
        FVertex{ .Position = { -0.5f,  0.5f,  0.5f }, .Normal = -XNormal, . UV = { 1.0f, 0.0f } },
        FVertex{ .Position = { -0.5f, -0.5f,  0.5f }, .Normal = -XNormal, . UV = { 0.0f, 0.0f } },

        // +Y
        FVertex{ .Position = { -0.5f, 0.5f, -0.5f }, .Normal = YNormal, . UV = { 0.0f, 1.0f } },
        FVertex{ .Position = {  0.5f, 0.5f, -0.5f }, .Normal = YNormal, . UV = { 1.0f, 1.0f } },
        FVertex{ .Position = {  0.5f, 0.5f,  0.5f }, .Normal = YNormal, . UV = { 1.0f, 0.0f } },
        FVertex{ .Position = { -0.5f, 0.5f,  0.5f }, .Normal = YNormal, . UV = { 0.0f, 0.0f } },

        // -Y
        FVertex{.Position = { -0.5f, -0.5f, -0.5f }, .Normal = -YNormal, . UV = { 0.0f, 1.0f } },
        FVertex{.Position = {  0.5f, -0.5f, -0.5f }, .Normal = -YNormal, . UV = { 1.0f, 1.0f } },
        FVertex{.Position = {  0.5f, -0.5f,  0.5f }, .Normal = -YNormal, . UV = { 1.0f, 0.0f } },
        FVertex{.Position = { -0.5f, -0.5f,  0.5f }, .Normal = -YNormal, . UV = { 0.0f, 0.0f } },
    };
    CubeGeometry.Indices =
    {
        FTriangle{ 0, 1, 2 },
        FTriangle{ 2, 3, 0 },

        FTriangle{ 4, 5, 6 },
        FTriangle{ 6, 7, 4 },

        FTriangle{ 8, 9, 10 },
        FTriangle{ 10, 11, 8 },

        FTriangle{ 12, 13, 14 },
        FTriangle{ 14, 15, 12 },

        FTriangle{ 16, 17, 18 },
        FTriangle{ 18, 19, 16 },

        FTriangle{ 20, 21, 22 },
        FTriangle{ 22, 23, 20 },
    };

    return CubeGeometry;
}

std::vector<glm::mat4> GenerateInstances(GLuint InNumInstances)
{
    std::vector<glm::mat4> ModelMatrices;
    ModelMatrices.reserve(InNumInstances);

    std::random_device Device;
    std::default_random_engine Generator(Device());

    constexpr float Jitter = 0.1f;
    std::normal_distribution<float> JitterDistribution{ -Jitter, Jitter };
    std::normal_distribution<float> NormalDistribution(0.0f, 0.1f);

    for (std::uint32_t Index = 0; Index < InNumInstances; ++Index)
    {
        const float Radius = 5.0f + JitterDistribution(Generator);

        const float Alpha = static_cast<float>(Index) / static_cast<float>(InNumInstances);
        const float Angle = glm::mix(0.0f, glm::two_pi<float>(), Alpha);

        const float X = Radius * glm::sin(Angle);
        // const float Y = NormalDistribution(Generator);
        float Y = Index / (float) InNumInstances;
        Y += NormalDistribution(Generator) * 0.5f;
        const float Z = Radius * glm::cos(Angle);

        glm::mat4 InstanceMatrix = glm::translate(glm::identity<glm::mat4>(), { X, Y, Z });
        constexpr float Scale = 0.01f;
        InstanceMatrix = glm::scale(InstanceMatrix, { Scale, Scale, Scale });

        ModelMatrices.emplace_back(InstanceMatrix);
    }

    return ModelMatrices;
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

std::map<std::string, GLint> LoadTextures(const std::vector<std::string>& InTextureFiles)
{
    struct TextureData
    {
        std::int32_t TextureWidth = 0;
        std::int32_t TextureHeight = 0;
        std::uint8_t* Data = nullptr;
    };

    std::mutex PrintMutex;

    std::vector<std::thread> Threads;
    std::map<std::string, std::future<TextureData>> TextureFutures;
    for (const std::string& TextureFile : InTextureFiles)
    {
        std::packaged_task<TextureData()> LoadTextureTask([&TextureFile, &PrintMutex]
        {
            TextureData Data;
            constexpr std::int32_t NumReqComponents = 3;
            Data.Data = stbi_load(TextureFile.c_str(), &Data.TextureWidth, &Data.TextureHeight, 0, NumReqComponents);
            assert(Data.Data);
            return Data;
        });

        TextureFutures.emplace(TextureFile, LoadTextureTask.get_future());

        Threads.emplace_back(std::move(LoadTextureTask));
    }

    std::map<std::string, GLint> LoadedTextures;
    for (std::pair<const std::string, std::future<TextureData>>& TextureFuturePair : TextureFutures)
    {
        std::string TextureFile = TextureFuturePair.first;
        TextureData Data = std::move(TextureFuturePair.second).get();

        // Gerar o Identifador da Textura
        GLuint TextureId;
        glGenTextures(1, &TextureId);

        // Habilita a textura para ser modificada
        glBindTexture(GL_TEXTURE_2D, TextureId);

        // Copia a textura para a memória da GPU
        const GLint Level = 0;
        const GLint Border = 0;
        glTexImage2D(GL_TEXTURE_2D, Level, GL_RGB, Data.TextureWidth, Data.TextureHeight, Border, GL_RGB, GL_UNSIGNED_BYTE, Data.Data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(Data.Data);
        LoadedTextures[TextureFile] = TextureId;
    }

    for (std::thread& Thread : Threads)
    {
        Thread.join();
    }

    return LoadedTextures;
}

void MouseButtonCallback(GLFWwindow* Window, std::int32_t Button, std::int32_t Action, std::int32_t Modifiers)
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    if (Button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (Action == GLFW_PRESS)
        {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            double X, Y;
            glfwGetCursorPos(Window, &X, &Y);

            gConfig.Scene.Camera.PreviousCursor = glm::vec2{ X, Y };
            gConfig.Scene.Camera.bEnableMouseMovement = true;
        }
        else if (Action == GLFW_RELEASE)
        {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            gConfig.Scene.Camera.bEnableMouseMovement = false;
        }
    }
}

void MouseMotionCallback(GLFWwindow* Window, double X, double Y)
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    glm::ivec2 CurrentPos = { static_cast<std::int32_t>(X), static_cast<std::int32_t>(Y) };

    if (gConfig.Input.Mouse.PreviousMousePos == glm::ivec2{ -1, -1 })
    {
        gConfig.Input.Mouse.PreviousMousePos = { static_cast<std::int32_t>(X), static_cast<std::int32_t>(Y) };
    }
    else
    {
        gConfig.Input.Mouse.MousePos = { static_cast<std::int32_t>(X), static_cast<std::int32_t>(Y) };
        gConfig.Input.Mouse.MouseDelta = gConfig.Input.Mouse.MousePos - gConfig.Input.Mouse.PreviousMousePos;
        gConfig.Input.Mouse.PreviousMousePos = gConfig.Input.Mouse.MousePos;
    }

    gConfig.Scene.Camera.MouseMove(static_cast<float>(X), static_cast<float>(Y));

    if (gConfig.Scene.SceneType == ESceneType::Ortho)
    {
        gConfig.Scene.PointLight.Position.x = static_cast<float>(X) / gConfig.Viewport.WindowWidth;
        gConfig.Scene.PointLight.Position.y = static_cast<float>(gConfig.Viewport.WindowHeight - Y) / gConfig.Viewport.WindowHeight;
    }
}

void KeyCallback(GLFWwindow* Window, std::int32_t Key, std::int32_t ScanCode, std::int32_t Action, std::int32_t Modifers)
{
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }

    if (Action == GLFW_PRESS)
    {
        switch (Key)
        {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(Window, true);
                break;

            case GLFW_KEY_W:
                gConfig.Scene.Camera.MoveForward(1.0f);
                break;

            case GLFW_KEY_S:
                gConfig.Scene.Camera.MoveForward(-1.0f);
                break;

            case GLFW_KEY_A:
                gConfig.Scene.Camera.MoveRight(-1.0f);
                break;

            case GLFW_KEY_D:
                gConfig.Scene.Camera.MoveRight(1.0f);
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
                gConfig.Scene.Camera.MoveForward(0.0f);
                break;

            case GLFW_KEY_S:
                gConfig.Scene.Camera.MoveForward(0.0f);
                break;

            case GLFW_KEY_A:
                gConfig.Scene.Camera.MoveRight(0.0f);
                break;

            case GLFW_KEY_D:
                gConfig.Scene.Camera.MoveRight(0.0f);
                break;

            case GLFW_KEY_O:
                gConfig.Scene.Camera.bIsOrtho = !gConfig.Scene.Camera.bIsOrtho;
                break;

            case GLFW_KEY_R:
                gConfig.Scene.Camera.Reset();
                break;

            case GLFW_KEY_M:
                gConfig.Render.bShowWireframe = !gConfig.Render.bShowWireframe;
                break;

            case GLFW_KEY_N:
                gConfig.Render.bCullFace = !gConfig.Render.bCullFace;
                break;

            case GLFW_KEY_P:
                gConfig.Simulation.bPause = !gConfig.Simulation.bPause;
                break;

            case GLFW_KEY_L:
                gConfig.Simulation.bReverse = !gConfig.Simulation.bReverse;
                break;

            default:
                break;
        }
    }
}

void ResizeCallback(GLFWwindow* Window, std::int32_t Width, std::int32_t Height)
{
    gConfig.Viewport.WindowWidth = Width;
    gConfig.Viewport.WindowHeight = Height;

    gConfig.Scene.Camera.SetViewportSize(Width, Height);

    glViewport(0, 0, Width, Height);
}

FRenderData GetRenderData()
{
    FGeometry Geo;
    FRenderData GeoRenderData;

    switch (gConfig.Scene.SceneType)
    {
        case ESceneType::BlueMarble:
            Geo = GenerateSphere(gConfig.Scene.SphereResolution);
            GeoRenderData.Transform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(180.0f), { 0.0f, 1.0f, 0.0f });
            gConfig.Scene.Camera.bIsOrtho = false;
            gConfig.Scene.PointLight.Position = { 0.0f, 0.0f, 1000.0f };
            gConfig.Scene.PointLight.Intensity = 1.0f;
            break;

        case ESceneType::Ortho:
            Geo = GenerateQuad();
            gConfig.Scene.Camera.bIsOrtho = true;
            gConfig.Scene.PointLight.Position = { 0.0f, 0.0f, 0.05f };
            gConfig.Scene.PointLight.Intensity = 1.0f;
            break;

        case ESceneType::Cylinder:
            Geo = GenerateCylinder(20);
            gConfig.Scene.Camera.bIsOrtho = false;
            gConfig.Scene.PointLight.Position = { 0.0f, 0.0f, 1000.0f };
            gConfig.Scene.PointLight.Intensity = 1.0f;
            break;

        case ESceneType::Cube:
            Geo = GenerateCube();
            gConfig.Scene.Camera.bIsOrtho = false;
            gConfig.Scene.PointLight.Position = { 0.0f, 0.0f, 1000.0f };
            gConfig.Scene.PointLight.Intensity = 1.0f;
            break;

        default:
            exit(1);
    }

    GLuint VertexBuffer, ElementBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Geo.Vertices.size() * sizeof(FVertex), Geo.Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ElementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Geo.Indices.size() * sizeof(FTriangle), Geo.Indices.data(), GL_STATIC_DRAW);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FVertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(FVertex), reinterpret_cast<void*>(offsetof(FVertex, Normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FVertex), reinterpret_cast<void*>(offsetof(FVertex, UV)));

    // Disabilitar o VAO
    glBindVertexArray(0);

    return GeoRenderData;
}

FRenderData GetAxisRenderData()
{
    const std::array<FLineVertex, 6> Vertices =
    {
        FLineVertex{.Position = { 0.0f, 0.0f, 0.0f }, .Color = { 1.0f, 0.0f, 0.0f } },
        FLineVertex{.Position = { 10.0f, 0.0f, 0.0f }, .Color = { 1.0f, 0.0f, 0.0f } },

        FLineVertex{.Position = { 0.0f, 0.0f, 0.0f }, .Color = { 0.0f, 1.0f, 0.0f } },
        FLineVertex{.Position = { 0.0f, 10.0f, 0.0f }, .Color = { 0.0f, 1.0f, 0.0f } },

        FLineVertex{.Position = { 0.0f, 0.0f, 0.0f }, .Color = { 0.0f, 0.0f, 1.0f } },
        FLineVertex{.Position = { 0.0f, 0.0f, 10.0f }, .Color = { 0.0f, 0.0f, 1.0f } },
    };

    GLuint VertexBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(FVertex), Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    FRenderData AxisRenderData;
    AxisRenderData.NumElements = (GLuint) Vertices.size();

    glGenVertexArrays(1, &AxisRenderData.VAO);
    glBindVertexArray(AxisRenderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FLineVertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(FLineVertex), reinterpret_cast<void*>(offsetof(FLineVertex, Color)));

    glBindVertexArray(0);

    return AxisRenderData;
}

FInstancedRenderData GetInstancedRenderData(std::int32_t InNumInstances)
{
    FGeometry Geo = GenerateSphere(10);
    std::vector<glm::mat4> Instances = GenerateInstances(InNumInstances);

    GLuint VertexBuffer, ElementBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Geo.Vertices.size() * sizeof(FVertex), Geo.Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ElementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Geo.Indices.size() * sizeof(FTriangle), Geo.Indices.data(), GL_STATIC_DRAW);
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FVertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(FVertex), reinterpret_cast<void*>(offsetof(FVertex, Normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FVertex), reinterpret_cast<void*>(offsetof(FVertex, UV)));

    // configura o array de matrizes nos atributos 3, 4, 5, 6
    glBindBuffer(GL_ARRAY_BUFFER, InstancesBuffer);

    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) 0);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (sizeof(glm::vec4)));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (2 * sizeof(glm::vec4)));
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (3 * sizeof(glm::vec4)));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);

    FInstancedRenderData InstRenderData;
    InstRenderData.VAO = InstanceVAO;
    InstRenderData.NumInstances = static_cast<GLuint>(Instances.size());
    InstRenderData.NumElements = static_cast<GLuint>(Geo.Indices.size()) * 3;
    return InstRenderData;
}


void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char* message,
                            const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

void DrawUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    ImGui::Begin("BlueMarble Config");
    {
        if (ImGui::CollapsingHeader("Render"))
        {
            ImGui::SeparatorText("Drawing");
            ImGui::Checkbox("Axis", &gConfig.Render.bDrawAxis);
            ImGui::Checkbox("Instances", &gConfig.Render.bDrawInstances);
            ImGui::Checkbox("Object", &gConfig.Render.bDrawObject);

            ImGui::SeparatorText("Rendering");
            ImGui::Checkbox("Cull Face", &gConfig.Render.bCullFace);
            ImGui::Checkbox("Wireframe", &gConfig.Render.bShowWireframe);
            ImGui::Checkbox("VSync", &gConfig.Render.bEnableVsync);
        }

        if (ImGui::CollapsingHeader("Simulation"))
        {
            ImGui::Checkbox("Pause", &gConfig.Simulation.bPause);
            ImGui::Checkbox("Reverse", &gConfig.Simulation.bReverse);

            ImGui::Text("Time (s)         : %f", gConfig.Simulation.TotalTime);
            ImGui::Text("Frame Count      : %d", gConfig.Simulation.FrameCount);
            ImGui::Text("Frmae Time (ms)  : %f", gConfig.Simulation.FrameTime);
            ImGui::Text("FPS              : %f", gConfig.Simulation.FramesPerSecond);
            ImGui::Text("Frames:          : %d", gConfig.Simulation.TotalFrames);

            if (ImGui::CollapsingHeader("Plots"))
            {
                const float AverageFrameTime = std::accumulate(gConfig.Simulation.FrameTimeHistory.begin(), gConfig.Simulation.FrameTimeHistory.end(), 0.0f) / gConfig.Simulation.FrameTimeHistory.size();
                const std::string AverageFrameTimeOverlay = "Avg: " + std::to_string(AverageFrameTime) + " ms";
                ImGui::PlotLines("Frame Times", gConfig.Simulation.FrameTimeHistory.data(), gConfig.Simulation.NumFramePlotValues, gConfig.Simulation.FramePlotOffset, AverageFrameTimeOverlay.c_str(), 0.0f, 100.0f, ImVec2(0, 100.0f));

                const float AvgFPS = std::accumulate(gConfig.Simulation.FramesPerSecondHistory.begin(), gConfig.Simulation.FramesPerSecondHistory.end(), 0.0f) / gConfig.Simulation.FramesPerSecondHistory.size();
                const std::string AverageFPSOverlay = "Avg:" + std::to_string(AvgFPS) + " ms";
                ImGui::PlotLines("FPS", gConfig.Simulation.FramesPerSecondHistory.data(), gConfig.Simulation.NumFramePlotValues, gConfig.Simulation.FramePlotOffset, AverageFPSOverlay.c_str(), 0.0f, 300.0f, ImVec2(0, 100.0f));
            }
        }

        if (ImGui::CollapsingHeader("Scene"))
        {
            ImGui::SeparatorText("Drawables");
            ImGui::DragInt("Num Instances", &gConfig.Scene.NumInstances, 1000.0f, 0, 1'000'000);

            ImGui::SeparatorText("Camera");
            ImGui::DragFloat3("Camera Location", glm::value_ptr(gConfig.Scene.Camera.Location), 0.1f);
            ImGui::DragFloat3("Camera Direction", glm::value_ptr(gConfig.Scene.Camera.Direction), 0.05f);
            ImGui::DragFloat3("Camera Orientation", glm::value_ptr(gConfig.Scene.Camera.Up), 0.05f);
            ImGui::DragFloat("Camera Near", &gConfig.Scene.Camera.Near, 0.05f, 0.0001f);
            ImGui::DragFloat("Camera Far", &gConfig.Scene.Camera.Far, 0.05f, 0.0001f);
            ImGui::DragFloat("Camera Field Of View", &gConfig.Scene.Camera.FieldOfView, 0.05f, 0.1f);
            ImGui::Checkbox("Camera Orthographic", &gConfig.Scene.Camera.bIsOrtho);

            ImGui::SeparatorText("Light");
            ImGui::DragFloat3("Light Position", glm::value_ptr(gConfig.Scene.PointLight.Position), 0.1f);
            ImGui::DragFloat("Intensity", &gConfig.Scene.PointLight.Intensity, 0.1f);
        }

        if (ImGui::CollapsingHeader("Viewport"))
        {
            ImGui::SeparatorText("Window");
            ImGui::Text("Width  : %d", gConfig.Viewport.WindowWidth);
            ImGui::Text("Height : %d", gConfig.Viewport.WindowHeight);
        }

        if (ImGui::CollapsingHeader("Input"))
        {
            ImGui::Text("Mouse Position: (%d, %d)", gConfig.Input.Mouse.MousePos.x, gConfig.Input.Mouse.MousePos.y);
            ImGui::Text("Mouse Delta   : (%d, %d)", gConfig.Input.Mouse.MouseDelta.x, gConfig.Input.Mouse.MouseDelta.y);
        }
    }
    ImGui::End();

    const std::map<std::filesystem::path, std::string>& ShaderFailureLogs = gConfig.Render.ShaderManager.GetFailureLogs();
    if (!ShaderFailureLogs.empty())
    {
        ImGui::Begin("Shader Compilation Logs");
        {
            std::stringstream LogStream;
            for (auto [ShaderFilePath, FailureLog] : ShaderFailureLogs)
            {
                LogStream << ShaderFilePath << "\n\n";
                LogStream << FailureLog << "\n\n";
            }
            ImGui::TextWrapped(LogStream.str().c_str());
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    if (!glfwInit())
    {
        std::cout << "Erro ao inicializar o GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    gConfig.Viewport.Window = glfwCreateWindow(gConfig.Viewport.WindowWidth, gConfig.Viewport.WindowHeight, "BlueMarble", nullptr, nullptr);
    if (!gConfig.Viewport.Window)
    {
        std::cout << "Erro ao criar janela" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetFramebufferSizeCallback(gConfig.Viewport.Window, ResizeCallback);
    glfwSetMouseButtonCallback(gConfig.Viewport.Window, MouseButtonCallback);
    glfwSetCursorPosCallback(gConfig.Viewport.Window, MouseMotionCallback);
    glfwSetKeyCallback(gConfig.Viewport.Window, KeyCallback);

    glfwMakeContextCurrent(gConfig.Viewport.Window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Io = ImGui::GetIO();
    Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    Io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    constexpr bool bInstallCallbacks = true;
    ImGui_ImplGlfw_InitForOpenGL(gConfig.Viewport.Window, bInstallCallbacks);
    ImGui_ImplOpenGL3_Init();

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Erro ao inicializar o GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLint ContextFlags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &ContextFlags);
    if (ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
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
    std::cout << "glfw Version    : " << glfwGetVersionString() << std::endl;
    std::cout << "ImGui Version   : " << IMGUI_VERSION << std::endl;

    std::shared_ptr<GLuint> ProgramId = gConfig.Render.ShaderManager.AddShader("triangle.vert", "triangle.frag");
    std::shared_ptr<GLuint> InstancedProgramId = gConfig.Render.ShaderManager.AddShader("instanced.vert", "instanced.frag");
    std::shared_ptr<GLuint> AxisProgramId = gConfig.Render.ShaderManager.AddShader("lines.vert", "lines.frag");

    FRenderData AxisRenderData = GetAxisRenderData();
    FRenderData GeoRenderData = GetRenderData();
    FInstancedRenderData InstRenderData = GetInstancedRenderData(gConfig.Scene.NumInstances);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLint EarthTextureId, CloudsTextureId;

    const std::string EarthTextureFile = "textures/earth_2k.jpg";
    const std::string CloudsTextureFile = "textures/earth_clouds_2k.jpg";

    const double LoadTexturesStartTime = glfwGetTime();
    constexpr bool bParallelLoadTextures = true;
    if constexpr (bParallelLoadTextures)
    {
        // Não vale a pena fazer isso só com duas texturas mas eu quis fazer uma função que fizesse isso de forma paralela mesmo assim
        const std::map<std::string, GLint> TextureIds = LoadTextures({ EarthTextureFile, CloudsTextureFile });
        EarthTextureId = TextureIds.at(EarthTextureFile);
        CloudsTextureId = TextureIds.at(CloudsTextureFile);
    }
    else
    {
        // Carregar a Textura para a Memória de Vídeo
        EarthTextureId = LoadTexture(EarthTextureFile.c_str());
        CloudsTextureId = LoadTexture(CloudsTextureFile.c_str());
    }
    const double LoadTexturesEndTime = glfwGetTime();
    std::cout << "Texturas Carregadas em " << (LoadTexturesEndTime - LoadTexturesStartTime) << " segundos" << std::endl;

    GLuint MatricesUBO, LightUBO;
    glGenBuffers(1, &MatricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, MatricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FUBOMatrices), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &LightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, LightUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FLight), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, MatricesUBO, 0, sizeof(FUBOMatrices));
    glBindBufferRange(GL_UNIFORM_BUFFER, 1, LightUBO, 0, sizeof(FLight));

    // Configura a cor de fundo
    glClearColor(0.1f, 0.1f, 0.1f, 1.0);

    gConfig.Scene.Camera.SetViewportSize(gConfig.Viewport.WindowWidth, gConfig.Viewport.WindowHeight);

    gConfig.Simulation.FrameTimeHistory.resize(gConfig.Simulation.NumFramePlotValues);
    gConfig.Simulation.FramesPerSecondHistory.resize(gConfig.Simulation.NumFramePlotValues);

    double TimeSinceLastFrame = 0.0f;
    double PreviousTime = glfwGetTime();

    while (!glfwWindowShouldClose(gConfig.Viewport.Window))
    {
        gConfig.Render.ShaderManager.UpdateShaders();

        {
            const GLuint MatricesUBOIndex = glGetUniformBlockIndex(*ProgramId, "Matrices");
            const GLuint LightUBOIndex = glGetUniformBlockIndex(*ProgramId, "LightUBO");

            glUniformBlockBinding(*ProgramId, MatricesUBOIndex, 0);
            glUniformBlockBinding(*ProgramId, LightUBOIndex, 1);
        }

        {
            const GLuint MatricesUBOIndex = glGetUniformBlockIndex(*InstancedProgramId, "Matrices");
            glUniformBlockBinding(*InstancedProgramId, MatricesUBOIndex, 0);
        }

        glfwPollEvents();

        glfwSwapInterval(gConfig.Render.bEnableVsync);

        if (gConfig.Render.bCullFace)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        const double CurrentTime = glfwGetTime();
        gConfig.Simulation.FrameTime = CurrentTime - PreviousTime;

        gConfig.Simulation.FrameTimeHistory.resize(gConfig.Simulation.NumFramePlotValues);
        gConfig.Simulation.FramesPerSecondHistory.resize(gConfig.Simulation.NumFramePlotValues);

        gConfig.Simulation.FrameTimeHistory[gConfig.Simulation.FramePlotOffset] = static_cast<float>(gConfig.Simulation.FrameTime) * 1000.0f;
        gConfig.Simulation.FramesPerSecondHistory[gConfig.Simulation.FramePlotOffset] = static_cast<float>(gConfig.Simulation.FramesPerSecond);

        gConfig.Simulation.FramePlotOffset = (gConfig.Simulation.FramePlotOffset + 1) % gConfig.Simulation.NumFramePlotValues;

        if (gConfig.Simulation.FrameTime > 0.0)
        {
            const double TimeScale = gConfig.Simulation.bReverse ? -1.0 : 1.0;
            gConfig.Simulation.TotalTime += gConfig.Simulation.FrameTime * (gConfig.Simulation.bPause ? 0.0f : TimeScale);
            TimeSinceLastFrame += gConfig.Simulation.FrameTime;
            if (TimeSinceLastFrame >= 1.0f)
            {
                gConfig.Simulation.FramesPerSecond = gConfig.Simulation.FrameCount / TimeSinceLastFrame;
                TimeSinceLastFrame = 0.0;
                gConfig.Simulation.FrameCount = 0;
                const std::string WindowTitle = "BlueMarble - FPS: " + std::to_string(gConfig.Simulation.FramesPerSecond);
                glfwSetWindowTitle(gConfig.Viewport.Window, WindowTitle.c_str());
            }

            gConfig.Scene.Camera.Update(static_cast<float>(gConfig.Simulation.FrameTime));
            PreviousTime = CurrentTime;
        }

        gConfig.Simulation.FrameCount++;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        FUBOMatrices Matrices = { .View = gConfig.Scene.Camera.GetView(),
            .Projection = gConfig.Scene.Camera.GetProjection() };

        glBindBuffer(GL_UNIFORM_BUFFER, MatricesUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(FUBOMatrices), &Matrices, GL_STATIC_DRAW);

        glBindBuffer(GL_UNIFORM_BUFFER, LightUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(FLight), &gConfig.Scene.PointLight, GL_STATIC_DRAW);

        if (gConfig.Render.bDrawAxis)
        {
            glUseProgram(*AxisProgramId);

            const glm::mat4 ModelViewProjectionMatrix = Matrices.Projection * Matrices.View;

            GLint ModelViewProjectionLoc = glGetUniformLocation(*AxisProgramId, "ModelViewProjection");
            glUniformMatrix4fv(ModelViewProjectionLoc, 1, GL_FALSE, glm::value_ptr(ModelViewProjectionMatrix));

            glBindVertexArray(AxisRenderData.VAO);
            glDrawArrays(GL_LINES, 0, AxisRenderData.NumElements);
            glBindVertexArray(0);
        }

        if (gConfig.Render.bDrawObject)
        {
            glUseProgram(*ProgramId);

            const glm::mat4 ModelMatrix = GeoRenderData.Transform;
            const glm::mat4 NormalMatrix = glm::transpose(glm::inverse(ModelMatrix));

            GLint TimeLoc = glGetUniformLocation(*ProgramId, "Time");
            glUniform1f(TimeLoc, static_cast<GLfloat>(gConfig.Simulation.TotalTime));

            GLint NormalMatrixLoc = glGetUniformLocation(*ProgramId, "NormalMatrix");
            glUniformMatrix4fv(NormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(NormalMatrix));

            GLint ModelMatrixLoc = glGetUniformLocation(*ProgramId, "ModelMatrix");
            glUniformMatrix4fv(ModelMatrixLoc, 1, GL_FALSE, glm::value_ptr(ModelMatrix));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, EarthTextureId);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, CloudsTextureId);

            GLint TextureSamplerLoc = glGetUniformLocation(*ProgramId, "EarthTexture");
            glUniform1i(TextureSamplerLoc, 0);

            GLint CloudsTextureSamplerLoc = glGetUniformLocation(*ProgramId, "CloudsTexture");
            glUniform1i(CloudsTextureSamplerLoc, 1);

            glPolygonMode(GL_FRONT_AND_BACK, gConfig.Render.bShowWireframe ? GL_LINE : GL_FILL);
            glBindVertexArray(GeoRenderData.VAO);
            glDrawElements(GL_TRIANGLES, GeoRenderData.NumElements, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        if (gConfig.Render.bDrawInstances)
        {
            // Render Instanced Data
            glUseProgram(*InstancedProgramId);

            GLuint TimeLoc = glGetUniformLocation(*InstancedProgramId, "Time");
            glUniform1f(TimeLoc, static_cast<GLfloat>(gConfig.Simulation.TotalTime));

            GLint NumInstancesLoc = glGetUniformLocation(*InstancedProgramId, "NumInstances");
            glUniform1i(NumInstancesLoc, InstRenderData.NumInstances);

            GLint TextureSamplerLoc = glGetUniformLocation(*InstancedProgramId, "EarthTexture");
            glUniform1i(TextureSamplerLoc, 0);

            GLint CloudsTextureSamplerLoc = glGetUniformLocation(*InstancedProgramId, "CloudsTexture");
            glUniform1i(CloudsTextureSamplerLoc, 1);

            glPolygonMode(GL_FRONT_AND_BACK, gConfig.Render.bShowWireframe ? GL_LINE : GL_FILL);
            glBindVertexArray(InstRenderData.VAO);
            glDrawElementsInstanced(GL_TRIANGLES, InstRenderData.NumElements, GL_UNSIGNED_INT, nullptr, std::min(static_cast<GLuint>(InstRenderData.NumInstances), static_cast<GLuint>(gConfig.Scene.NumInstances)));
            glBindVertexArray(0);
        }

        glUseProgram(0);

        DrawUI();

        glfwSwapBuffers(gConfig.Viewport.Window);

        // O Mouse Delta precisa ser resetado aqui ou ele fica com o valor acumulado do frame anterior
        gConfig.Input.Mouse.MouseDelta = { 0, 0 };
    }

    glfwDestroyWindow(gConfig.Viewport.Window);
    glfwTerminate();

    return 0;
}
