#pragma once

#include "DirectoryWatcher.h"

#include <glad/glad.h>

#include <filesystem>
#include <map>
#include <memory>

class FShaderManager
{
public:

    std::shared_ptr<GLuint> AddShader(const std::string& InVertexShaderFile, const std::string& InFragmentShaderFile);

    const std::map<std::filesystem::path, std::string> GetFailureLogs() const { return FailureLogs; }

    void UpdateShaders();

private:

    bool IsShaderValid(GLuint InShaderId, std::string& OutInfoLog);

    bool IsProgramValid(GLuint InProgramId);

    bool CompileAndLink(GLuint InProgramId, const std::filesystem::path& InVertexShaderFile, const std::filesystem::path& InFragmentShaderFile);

private:
    static constexpr std::string_view ShadersDir = "../../../shaders";
    FDirectoryWatcher DirWatcher{ ShadersDir };
    std::map<std::shared_ptr<GLuint>, std::pair<std::filesystem::path, std::filesystem::path>> ShaderMap;
    std::map<std::filesystem::path, std::string> FailureLogs;
};
