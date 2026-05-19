#include "uwupch.h"
#include "TextureLoader.h"

#ifdef UWU_HAS_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

namespace UwU_Engine
{
    bool TextureLoader::Load(const std::string& path, TextureAsset& out, std::string& error)
    {
#ifndef UWU_HAS_STB_IMAGE
        error = "stb_image is not configured. Put stb_image.h into ExternalLibs/stb_image.";
        return false;
#else
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
        {
            error = stbi_failure_reason() ? stbi_failure_reason() : "stbi_load failed";
            return false;
        }

        const int outputChannels = 4;
        const size_t byteCount = static_cast<size_t>(width) * static_cast<size_t>(height)
            * static_cast<size_t>(outputChannels);

        out.texture.width = width;
        out.texture.height = height;
        out.texture.channels = outputChannels;
        out.texture.pixels.assign(pixels, pixels + byteCount);
        stbi_image_free(pixels);

        UWU_ENGINE_INFO("[TextureLoader] Loaded '{}' ({}x{}, {} channels)",
            path, width, height, outputChannels);
        return true;
#endif
    }
}
