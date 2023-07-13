
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

bool FShaderManager::CompileAndLink(FShaderPtr InShader)
{
    // Criar os identificadores de cada um dos shaders
    const GLuint VertShaderId = glCreateShader(GL_VERTEX_SHADER);
    const GLuint FragShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    const std::string VertexShaderSource = ReadFile(InShader->VertexShaderFilePath);
    const std::string FragmentShaderSource = ReadFile(InShader->FragmentShaderFilePath);

    if (VertexShaderSource.empty() || FragmentShaderSource.empty())
    {
        return false;
    }

    std::cout << "Compilando " << InShader->VertexShaderFilePath << std::endl;
    const char* VertexShaderSourcePtr = VertexShaderSource.c_str();
    glShaderSource(VertShaderId, 1, &VertexShaderSourcePtr, nullptr);
    glCompileShader(VertShaderId);

    std::cout << "Compilando " << InShader->FragmentShaderFilePath << std::endl;
    const char* FragmentShaderSourcePtr = FragmentShaderSource.c_str();
    glShaderSource(FragShaderId, 1, &FragmentShaderSourcePtr, nullptr);
    glCompileShader(FragShaderId);

    std::string VertexShaderInfoLog, FragmentShaderInfoLog;
    if (IsShaderValid(VertShaderId, VertexShaderInfoLog) && IsShaderValid(FragShaderId, FragmentShaderInfoLog))
    {
        const GLint ProgramId = glCreateProgram();

        std::cout << "Linkando Programa" << std::endl;
        glAttachShader(ProgramId, VertShaderId);
        glAttachShader(ProgramId, FragShaderId);
        glLinkProgram(ProgramId);

        glDetachShader(ProgramId, VertShaderId);
        glDetachShader(ProgramId, FragShaderId);

        glDeleteShader(VertShaderId);
        glDeleteShader(FragShaderId);

        if (IsProgramValid(ProgramId))
        {
            GLint NumUniforms = 0, MaxUniformNameLength = 0;
            glGetProgramiv(ProgramId, GL_ACTIVE_UNIFORMS, &NumUniforms);
            glGetProgramiv(ProgramId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &MaxUniformNameLength);

            InShader->UniformBlockBindings.clear();
            InShader->UniformLocations.clear();

            for (GLint UniformIndex = 0; UniformIndex < NumUniforms; ++UniformIndex)
            {
                std::string UniformNameBuffer(MaxUniformNameLength, '\0');
                GLsizei UniformNameLength = 0;
                GLint UniformSize = 0;
                GLenum UniformType;
                glGetActiveUniform(ProgramId, UniformIndex, MaxUniformNameLength, &UniformNameLength, &UniformSize, &UniformType, UniformNameBuffer.data());

                const std::string UniformName = UniformNameBuffer.substr(0, UniformNameLength);
                const GLint UniformLoc = glGetUniformLocation(ProgramId, UniformName.c_str());

                InShader->UniformLocations[UniformName] = UniformLoc;
            }


            GLint NumUniformBlocks = 0, MaxUniformBlockNameLength = 0;
            glGetProgramiv(ProgramId, GL_ACTIVE_UNIFORM_BLOCKS, &NumUniformBlocks);
            glGetProgramiv(ProgramId, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &MaxUniformBlockNameLength);

            for (GLint UBOIndex = 0; UBOIndex < NumUniformBlocks; ++UBOIndex)
            {
                std::string UniformBlockNameBuffer(MaxUniformBlockNameLength, '\0');
                GLsizei UniformBlockNameLength = 0;
                glGetActiveUniformBlockName(ProgramId, UBOIndex, MaxUniformBlockNameLength, &UniformBlockNameLength, UniformBlockNameBuffer.data());

                const std::string UniformBlockName = UniformBlockNameBuffer.substr(0, UniformBlockNameLength);
                glUniformBlockBinding(ProgramId, UBOIndex, UBOIndex);

                InShader->UniformBlockBindings[UniformBlockName] = UBOIndex;
            }

            InShader->ProgramId = ProgramId;

            return true;
        }
    }
    else
    {
        if (!VertexShaderInfoLog.empty())
        {
            FailureLogs[InShader->VertexShaderFilePath] = VertexShaderInfoLog;
        }

        if (!FragmentShaderSource.empty())
        {
            FailureLogs[InShader->FragmentShaderFilePath] = FragmentShaderInfoLog;
        }
    }

    return false;
}

FShaderPtr FShaderManager::AddShader(const std::string& InVertexShaderFile, const std::string& InFragmentShaderFile)
{
    const std::filesystem::path AbsoluteVertexShaderFile = std::filesystem::absolute(std::filesystem::path{ ShadersDir } / InVertexShaderFile);
    const std::filesystem::path AbsoluteFragShaderFile = std::filesystem::absolute(std::filesystem::path{ ShadersDir } / InFragmentShaderFile);

    FShaderPtr Shader = std::make_shared<FShader>();
    Shader->VertexShaderFilePath = AbsoluteVertexShaderFile;
    Shader->FragmentShaderFilePath = AbsoluteFragShaderFile;

    if (CompileAndLink(Shader))
    {
        Shaders.push_back(Shader);
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
            auto ShaderIt = std::find_if(Shaders.begin(), Shaders.end(), [ShaderFile](FShaderPtr Shader)
            {
                return Shader->VertexShaderFilePath == ShaderFile || Shader->FragmentShaderFilePath == ShaderFile;
            });
            if (ShaderIt != Shaders.end())
            {
                FShaderPtr Shader = *ShaderIt;

                const GLint PreviousProgramId = Shader->ProgramId;
                if (CompileAndLink(Shader))
                {
                    glDeleteProgram(PreviousProgramId);
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