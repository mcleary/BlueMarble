#include <array>
#include <iostream>
#include <fstream>
#include <vector>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Camera.h"

GLFWwindow* Window = nullptr;
constexpr std::int32_t WindowWidth = 1280;
constexpr std::int32_t WindowHeight = 720;

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Color;
    glm::vec2 UV;
};

struct Triangle
{
    GLuint V0;
    GLuint V1;
    GLuint V2;
};

struct DirectionalLight
{
    glm::vec3 Direction;
    GLfloat Intensity;
};

SimpleCamera Camera;

void GenerateSphere(GLuint Resolution, std::vector<Vertex>& Vertices, std::vector<Triangle>& Indices)
{
    Vertices.clear();
    Indices.clear();

    constexpr float Pi = glm::pi<float>();
    constexpr float TwoPi = glm::two_pi<float>();
    float InvResolution = 1.0f / static_cast<float>(Resolution - 1);

    for (GLuint UIndex = 0; UIndex < Resolution; ++UIndex)
    {
        const float U = UIndex * InvResolution;
        const float Theta = glm::mix(0.0f, TwoPi, static_cast<float>(U));

        for (GLuint VIndex = 0; VIndex < Resolution; ++VIndex)
        {
            const float V = VIndex * InvResolution;
            const float Phi = glm::mix(0.0f, Pi, static_cast<float>(V));

            glm::vec3 VertexPosition =
            {
                glm::cos(Theta) * glm::sin(Phi),
                glm::sin(Theta) * glm::sin(Phi),
                glm::cos(Phi)
            };

            glm::vec3 VertexNormal = glm::normalize(VertexPosition);

            Vertices.push_back(Vertex{
                VertexPosition,
                VertexNormal,
                glm::vec3{ 1.0f, 1.0f, 1.0f },
                glm::vec2{ 1.0f - U, 1.0f - V }
                               });
        }
    }

    for (GLuint U = 0; U < Resolution - 1; ++U)
    {
        for (GLuint V = 0; V < Resolution - 1; ++V)
        {
            GLuint P0 = U + V * Resolution;
            GLuint P1 = U + 1 + V * Resolution;
            GLuint P2 = U + (V + 1) * Resolution;
            GLuint P3 = U + 1 + (V + 1) * Resolution;

            Indices.push_back(Triangle{ P3, P2, P0 });
            Indices.push_back(Triangle{ P1, P3, P0 });
        }
    }
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

        std::string ShaderInfoLog(InfoLogLength, '\0');
        glGetShaderInfoLog(ShaderId, InfoLogLength, nullptr, &ShaderInfoLog[0]);

        if (InfoLogLength > 0)
        {
            std::cout << "Erro no Vertex Shader: " << std::endl;
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

void MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Modifiers)
{
    // std::cout << "Button: " << Button << " Action: " << Action << " Modifiers: " << Modifiers << std::endl;

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

            default:
                break;
        }
    }
}

void ResizeCallback(GLFWwindow* Window, std::int32_t Width, std::int32_t Height)
{
    Camera.SetViewportSize(Width, Height);

    glViewport(0, 0, Width, Height);
}

void BlueMarbleScene()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    // Compilar o vertex e o fragment shader
    GLuint ProgramId = LoadShaders("shaders/triangle_vert.glsl", "shaders/triangle_frag.glsl");

    // Gera a Geometria da esfera e copia os dados para a GPU (memória da placa de vídeo)
    std::vector<Vertex> SphereVertices;
    std::vector<Triangle> SphereIndices;
    GenerateSphere(100, SphereVertices, SphereIndices);

    const std::int32_t NumTriangles = static_cast<std::int32_t>(SphereIndices.size());

    // Copia os dados da esfera para a GPU
    GLuint SphereVertexBuffer, SphereElementBuffer;
    glGenBuffers(1, &SphereVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, SphereVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, SphereVertices.size() * sizeof(Vertex), SphereVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &SphereElementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SphereElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, SphereIndices.size() * sizeof(Triangle), SphereIndices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Não precisa mais manter o vetor na CPU
    SphereIndices.clear();
    SphereVertices.clear();

    // Gerar o identificador do VAO
    // Identificador do Vertex Array Object (VAO)
    GLuint SphereVAO;
    glGenVertexArrays(1, &SphereVAO);
    glBindVertexArray(SphereVAO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // Diz para o OpenGL que o VertexBuffer vai ficar associado ao atributo 0
    // glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, SphereVertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SphereElementBuffer);

    // Informa ao OpenGL onde, dentro do VertexBuffer, os vértices estão. No
    // nosso caso o array Triangles é tudo o que a gente precisa
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Color)));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, UV)));

    // Disabilitar o VAO
    glBindVertexArray(0);

    // Criar uma fonte de luz direcional
    DirectionalLight Light;
    Light.Direction = glm::vec3(0.0f, 0.0f, -1.0f);
    Light.Intensity = 1.0f;

    glm::mat4 ModelMatrix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });

    // Carregar a Textura para a Memória de Vídeo
    GLuint EarthTextureId = LoadTexture("textures/earth_2k.jpg");
    GLuint CloudsTextureId = LoadTexture("textures/earth_clouds_2k.jpg");

    // Configura a cor de fundo
    glClearColor(0.0f, 0.0f, 0.0f, 1.0);

    Camera.SetViewportSize(WindowWidth, WindowHeight);

    double PreviousTime = glfwGetTime();

    while (!glfwWindowShouldClose(Window))
    {
        double CurrentTime = glfwGetTime();
        double DeltaTime = CurrentTime - PreviousTime;
        if (DeltaTime > 0.0)
        {
            Camera.Update(static_cast<float>(DeltaTime));
            PreviousTime = CurrentTime;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(ProgramId);

        glm::mat4 ViewMatrix = Camera.GetView();
        glm::mat4 NormalMatrix = glm::transpose(glm::inverse(ViewMatrix * ModelMatrix));
        glm::mat4 ModelViewMatrix = ViewMatrix * ModelMatrix;
        glm::mat4 ModelViewProjectionMatrix = Camera.GetViewProjection() * ModelMatrix;

        GLint TimeLoc = glGetUniformLocation(ProgramId, "Time");
        glUniform1f(TimeLoc, static_cast<GLfloat>(CurrentTime));

        GLint NormalMatrixLoc = glGetUniformLocation(ProgramId, "NormalMatrix");
        glUniformMatrix4fv(NormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(NormalMatrix));

        GLint ModelViewMatrixLoc = glGetUniformLocation(ProgramId, "ModelViewMatrix");
        glUniformMatrix4fv(ModelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(ModelViewMatrix));

        GLint ModelViewProjectionLoc = glGetUniformLocation(ProgramId, "ModelViewProjection");
        glUniformMatrix4fv(ModelViewProjectionLoc, 1, GL_FALSE, glm::value_ptr(ModelViewProjectionMatrix));

        GLint LightIntensityLoc = glGetUniformLocation(ProgramId, "LightIntensity");
        glUniform1f(LightIntensityLoc, Light.Intensity);

        glm::vec4 LightDirectionViewSpace = ViewMatrix * glm::vec4{ Light.Direction, 0.0f };

        GLint LightDirectionLoc = glGetUniformLocation(ProgramId, "LightDirection");
        glUniform3fv(LightDirectionLoc, 1, glm::value_ptr(LightDirectionViewSpace));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, EarthTextureId);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, CloudsTextureId);

        GLint TextureSamplerLoc = glGetUniformLocation(ProgramId, "EarthTexture");
        glUniform1i(TextureSamplerLoc, 0);

        GLint CloudsTextureSamplerLoc = glGetUniformLocation(ProgramId, "CloudsTexture");
        glUniform1i(CloudsTextureSamplerLoc, 1);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(SphereVAO);
        glDrawElements(GL_TRIANGLES, NumTriangles * 3, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }

    glDeleteBuffers(1, &SphereElementBuffer);
    glDeleteBuffers(1, &SphereVertexBuffer);
    glDeleteVertexArrays(1, &SphereVAO);
    glDeleteProgram(ProgramId);
    glDeleteTextures(1, &EarthTextureId);
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

    BlueMarbleScene();

    glfwDestroyWindow(Window);
    glfwTerminate();

    return 0;
}
