#pragma once
#include <string>

namespace Lumos
{
    void EmbedTexture(const std::string& texFilePath, const std::string& outPath, const std::string& arrayName);
    void EmbedShader(const std::string& shaderPath);

}
