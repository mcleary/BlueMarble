#pragma once

#include "DirectoryWatcher.h"

#include <glad/glad.h>

#include <filesystem>
#include <map>
#include <memory>

class FShaderManager
{
public:

    bool IsShaderValid(GLuint InShaderId);

    bool IsProgramValid(GLuint InProgramId);

    bool CompileAndLink(GLuint InProgramId, const std::filesystem::path& InVertexShaderFile, const std::filesystem::path& InFragmentShaderFile);

    std::shared_ptr<GLuint> AddShader(const std::string& InVertexShaderFile, const std::string& InFragmentShaderFile);

    void UpdateShaders();

private:
    static constexpr std::string_view ShadersDir = "../../../shaders";
    FDirectoryWatcher DirWatcher{ ShadersDir };
    std::map<std::shared_ptr<GLuint>, std::pair<std::filesystem::path, std::filesystem::path>> ShaderMap;
};
