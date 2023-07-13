#pragma once

#include "DirectoryWatcher.h"

#include <glad/glad.h>

#include <filesystem>
#include <map>
#include <memory>

struct FShader
{
    GLint ProgramId;
    std::filesystem::path VertexShaderFilePath;
    std::filesystem::path FragmentShaderFilePath;
    std::map<std::string, GLint> UniformBlockBindings;
    std::map<std::string, GLint> UniformLocations;
};

using FShaderPtr = std::shared_ptr<FShader>;

class FShaderManager
{
public:

    FShaderPtr AddShader(const std::string& InVertexShaderFile, const std::string& InFragmentShaderFile);

    const std::map<std::filesystem::path, std::string> GetFailureLogs() const { return FailureLogs; }

    void UpdateShaders();

private:

    bool IsShaderValid(GLuint InShaderId, std::string& OutInfoLog);

    bool IsProgramValid(GLuint InProgramId);

    bool CompileAndLink(FShaderPtr InShader);

private:
    static constexpr std::string_view ShadersDir = "../../../shaders";
    FDirectoryWatcher DirWatcher{ ShadersDir };
    std::vector<FShaderPtr> Shaders;
    std::map<std::filesystem::path, std::string> FailureLogs;
};
