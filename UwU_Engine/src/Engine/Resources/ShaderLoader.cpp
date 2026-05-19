#include "uwupch.h"
#include "ShaderLoader.h"

namespace UwU_Engine
{
    bool ShaderLoader::Load(const std::string& path, ShaderAsset& out, std::string& error)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            error = "cannot open shader file";
            return false;
        }

        std::ostringstream stream;
        stream << file.rdbuf();

        out.shader.sourcePath = path;
        out.shader.sourceCode = stream.str();

        if (out.shader.sourceCode.empty())
        {
            error = "shader file is empty";
            return false;
        }

        UWU_ENGINE_INFO("[ShaderLoader] Loaded '{}' ({} bytes)",
            path, out.shader.sourceCode.size());
        return true;
    }
}
