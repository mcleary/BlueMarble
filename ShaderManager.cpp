
#include "ShaderManager.h"

#include <array>
#include <fstream>
#include <iostream>

static std::string ReadFile(const std::filesystem::path & InFilePath)
{
    std::string FileContents;
    if (std::ifstream FileStream{ InFilePath.string(), std::ios::in })
    {
        FileContents.assign((std::istreambuf_iterator<char>(FileStream)), std::istreambuf_iterator<char>());
    }
    return FileContents;
}

bool FShaderManager::IsShaderValid(GLuint InShaderId, std::string& OutInfoLog)
{
    // Verificar se o shader foi compilado
    GLint Result = GL_TRUE;
    glGetShaderiv(InShaderId, GL_COMPILE_STATUS, &Result);

    if (Result == GL_FALSE)
    {
        // Erro ao compilar o shader, imprimir o log para saber o que está errado
        GLint InfoLogLength = 0;
        glGetShaderiv(InShaderId, GL_INFO_LOG_LENGTH, &InfoLogLength);

        if (InfoLogLength > 0)
        {
            GLint ShaderType = 0;
            glGetShaderiv(InShaderId, GL_SHADER_TYPE, &ShaderType);

            std::string ShaderInfoLog(InfoLogLength, '\0');
            glGetShaderInfoLog(InShaderId, InfoLogLength, nullptr, &ShaderInfoLog[0]);

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
                    ShaderTypeStr = "<Unknown>";
                    break;
            }

            std::cout << "Erro no " << ShaderTypeStr << " Shader: " << std::endl;
            std::cout << ShaderInfoLog << std::endl;
            OutInfoLog = ShaderInfoLog;

            return false;
        }
    }

    return true;
}

bool FShaderManager::IsProgramValid(GLuint InProgramId)
{
    // Verificar o programa
    GLint LinkStatus = GL_TRUE;
    GLint ValidateStatus = GL_TRUE;
    glGetProgramiv(InProgramId, GL_LINK_STATUS, &LinkStatus);
    glGetProgramiv(InProgramId, GL_VALIDATE_STATUS, &ValidateStatus);

    if (LinkStatus == GL_FALSE || ValidateStatus == GL_FALSE)
    {
        GLint InfoLogLength = 0;
        glGetProgramiv(InProgramId, GL_INFO_LOG_LENGTH, &InfoLogLength);

        if (InfoLogLength > 0)
        {
            std::string ProgramInfoLog(InfoLogLength, '\0');
            glGetProgramInfoLog(InProgramId, InfoLogLength, nullptr, &ProgramInfoLog[0]);

            std::cout << "Erro ao linkar programa" << std::endl;
            std::cout << ProgramInfoLog << std::endl;

            return false;
        }
    }

    return true;
}

bool FShaderManager::CompileAndLink(GLuint InProgramId, const std::filesystem::path& InVertexShaderFile, const std::filesystem::path& InFragmentShaderFile)
{
    // Criar os identificadores de cada um dos shaders
    const GLuint VertShaderId = glCreateShader(GL_VERTEX_SHADER);
    const GLuint FragShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    const std::string VertexShaderSource = ReadFile(InVertexShaderFile.string());
    const std::string FragmentShaderSource = ReadFile(InFragmentShaderFile.string());

    if (VertexShaderSource.empty() || FragmentShaderSource.empty())
    {
        return false;
    }

    std::cout << "Compilando " << InVertexShaderFile << std::endl;
    const char* VertexShaderSourcePtr = VertexShaderSource.c_str();
    glShaderSource(VertShaderId, 1, &VertexShaderSourcePtr, nullptr);
    glCompileShader(VertShaderId);

    std::cout << "Compilando " << InFragmentShaderFile << std::endl;
    const char* FragmentShaderSourcePtr = FragmentShaderSource.c_str();
    glShaderSource(FragShaderId, 1, &FragmentShaderSourcePtr, nullptr);
    glCompileShader(FragShaderId);

    std::string VertexShaderInfoLog, FragmentShaderInfoLog;
    if (IsShaderValid(VertShaderId, VertexShaderInfoLog) && IsShaderValid(FragShaderId, FragmentShaderInfoLog))
    {
        std::cout << "Linkando Programa" << std::endl;
        glAttachShader(InProgramId, VertShaderId);
        glAttachShader(InProgramId, FragShaderId);
        glLinkProgram(InProgramId);

        glDetachShader(InProgramId, VertShaderId);
        glDetachShader(InProgramId, FragShaderId);

        glDeleteShader(VertShaderId);
        glDeleteShader(FragShaderId);

        constexpr std::array<std::string_view, 3> UBONames = { "FrameUBO", "ModelUBO", "LightUBO" };

        GLint BindingIndex = 0;
        for (const std::string_view& UBOName : UBONames)
        {
            const GLint UBOIndex = glGetUniformBlockIndex(InProgramId, UBOName.data());
            if (UBOIndex != -1)
            {
                glUniformBlockBinding(InProgramId, UBOIndex, static_cast<GLint>(BindingIndex++));
            }
        }
        return IsProgramValid(InProgramId);
    }
    else
    {
        if (!VertexShaderInfoLog.empty())
        {
            FailureLogs[InVertexShaderFile] = VertexShaderInfoLog;
        }

        if (!FragmentShaderSource.empty())
        {
            FailureLogs[InFragmentShaderFile] = FragmentShaderInfoLog;
        }
    }

    return false;
}

std::shared_ptr<GLuint> FShaderManager::AddShader(const std::string& InVertexShaderFile, const std::string& InFragmentShaderFile)
{
    const std::filesystem::path AbsoluteVertexShaderFile = std::filesystem::absolute(std::filesystem::path{ ShadersDir } / InVertexShaderFile);
    const std::filesystem::path AbsoluteFragShaderFile = std::filesystem::absolute(std::filesystem::path{ ShadersDir } / InFragmentShaderFile);

    std::shared_ptr<GLuint> ProgramId = std::make_shared<GLuint>(glCreateProgram());

    if (CompileAndLink(*ProgramId, AbsoluteVertexShaderFile, AbsoluteFragShaderFile))
    {
        ShaderMap[ProgramId] = { AbsoluteVertexShaderFile, AbsoluteFragShaderFile };
    }

    return ProgramId;
}

void FShaderManager::UpdateShaders()
{
    std::set<std::filesystem::path> ChangedFiles = DirWatcher.GetChangedFiles();
    if (!ChangedFiles.empty())
    {
        FailureLogs.clear();

        for (const std::filesystem::path& ShaderFile : ChangedFiles)
        {
            auto ShaderIt = std::find_if(ShaderMap.begin(), ShaderMap.end(), [ShaderFile](const std::pair<std::shared_ptr<GLuint>, std::pair<std::filesystem::path, std::filesystem::path>>& ShaderMapPair)
            {
                const std::pair<std::filesystem::path, std::filesystem::path>& ShaderFilePair = ShaderMapPair.second;
                return ShaderFilePair.first == ShaderFile || ShaderFilePair.second == ShaderFile;
            });
            if (ShaderIt != ShaderMap.end())
            {
                std::shared_ptr<GLuint> ProgramId = ShaderIt->first;
                const std::filesystem::path& VertexShaderFile = ShaderIt->second.first;
                const std::filesystem::path& FragmentShaderFile = ShaderIt->second.second;

                const GLuint NewProgramid = glCreateProgram();
                if (CompileAndLink(NewProgramid, VertexShaderFile, FragmentShaderFile))
                {
                    glDeleteProgram(*ProgramId);
                    *ProgramId = NewProgramid;
                }
            }
        }
    }
}


#if 0
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

    return ProgramId;
}
#endif