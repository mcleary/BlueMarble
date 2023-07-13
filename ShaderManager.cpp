
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

bool FShaderManager::CompileAndLink(GLuint InProgramId, const std::filesystem::path& InVertexShaderFile, const std::filesystem::path& InFragmentShaderFile, std::map<std::string, GLint>& OutUniformLocations,  std::map<std::string, GLint>& OutUniformBlockBindings)
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

        if (IsProgramValid(InProgramId))
        {
            GLint NumUniforms = 0, MaxUniformNameLength = 0;
            glGetProgramiv(InProgramId, GL_ACTIVE_UNIFORMS, &NumUniforms);
            glGetProgramiv(InProgramId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &MaxUniformNameLength);

            for (GLint UniformIndex = 0; UniformIndex < NumUniforms; ++UniformIndex)
            {
                std::string UniformNameBuffer(MaxUniformNameLength, '\0');
                GLsizei UniformNameLength = 0;
                GLint UniformSize = 0;
                GLenum UniformType;
                glGetActiveUniform(InProgramId, UniformIndex, MaxUniformNameLength, &UniformNameLength, &UniformSize, &UniformType, UniformNameBuffer.data());

                const std::string UniformName = UniformNameBuffer.substr(0, UniformNameLength);
                const GLint UniformLoc = glGetUniformLocation(InProgramId, UniformName.c_str());

                OutUniformLocations[UniformName] = UniformLoc;
            }


            GLint NumUniformBlocks = 0, MaxUniformBlockNameLength = 0;
            glGetProgramiv(InProgramId, GL_ACTIVE_UNIFORM_BLOCKS, &NumUniformBlocks);
            glGetProgramiv(InProgramId, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &MaxUniformBlockNameLength);

            for (GLint UBOIndex = 0; UBOIndex < NumUniformBlocks; ++UBOIndex)
            {
                std::string UniformBlockNameBuffer(MaxUniformBlockNameLength, '\0');
                GLsizei UniformBlockNameLength = 0;
                glGetActiveUniformBlockName(InProgramId, UBOIndex, MaxUniformBlockNameLength, &UniformBlockNameLength, UniformBlockNameBuffer.data());

                const std::string UniformBlockName = UniformBlockNameBuffer.substr(0, UniformBlockNameLength);
                glUniformBlockBinding(InProgramId, UBOIndex, UBOIndex);

                OutUniformBlockBindings[UniformBlockName] = UBOIndex;
            }

            return true;
        }
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

FShaderPtr FShaderManager::AddShader(const std::string& InVertexShaderFile, const std::string& InFragmentShaderFile)
{
    const std::filesystem::path AbsoluteVertexShaderFile = std::filesystem::absolute(std::filesystem::path{ ShadersDir } / InVertexShaderFile);
    const std::filesystem::path AbsoluteFragShaderFile = std::filesystem::absolute(std::filesystem::path{ ShadersDir } / InFragmentShaderFile);

    FShaderPtr Shader = std::make_shared<FShader>();
    Shader->ProgramId = glCreateProgram();

    if (CompileAndLink(Shader->ProgramId, AbsoluteVertexShaderFile, AbsoluteFragShaderFile, Shader->UniformLocations, Shader->UniformBlockBindings))
    {
        ShaderMap[Shader] = { AbsoluteVertexShaderFile, AbsoluteFragShaderFile };
    }

    return Shader;
}

void FShaderManager::UpdateShaders()
{
    std::set<std::filesystem::path> ChangedFiles = DirWatcher.GetChangedFiles();
    if (!ChangedFiles.empty())
    {
        FailureLogs.clear();

        for (const std::filesystem::path& ShaderFile : ChangedFiles)
        {
            auto ShaderIt = std::find_if(ShaderMap.begin(), ShaderMap.end(), [ShaderFile](const std::pair<FShaderPtr, std::pair<std::filesystem::path, std::filesystem::path>>& ShaderMapPair)
            {
                const std::pair<std::filesystem::path, std::filesystem::path>& ShaderFilePair = ShaderMapPair.second;
                return ShaderFilePair.first == ShaderFile || ShaderFilePair.second == ShaderFile;
            });
            if (ShaderIt != ShaderMap.end())
            {
                FShaderPtr Shader = ShaderIt->first;
                const std::filesystem::path& VertexShaderFile = ShaderIt->second.first;
                const std::filesystem::path& FragmentShaderFile = ShaderIt->second.second;

                const GLuint NewProgramid = glCreateProgram();
                if (CompileAndLink(NewProgramid, VertexShaderFile, FragmentShaderFile, Shader->UniformLocations, Shader->UniformBlockBindings))
                {
                    glDeleteProgram(Shader->ProgramId);
                    Shader->ProgramId = NewProgramid;
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