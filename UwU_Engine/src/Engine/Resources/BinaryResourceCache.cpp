#include "uwupch.h"
#include "BinaryResourceCache.h"

namespace UwU_Engine
{
    namespace
    {
        constexpr std::array<char, 8> kMeshMagic = { 'U', 'W', 'U', 'M', 'E', 'S', 'H', '1' };
        constexpr std::array<char, 8> kTextureMagic = { 'U', 'W', 'U', 'T', 'E', 'X', '0', '1' };
        constexpr uint32_t kVersion = 1;

        template<typename T>
        bool WritePod(std::ofstream& file, const T& value)
        {
            file.write(reinterpret_cast<const char*>(&value), sizeof(T));
            return file.good();
        }

        template<typename T>
        bool ReadPod(std::ifstream& file, T& value)
        {
            file.read(reinterpret_cast<char*>(&value), sizeof(T));
            return file.good();
        }

        bool WriteString(std::ofstream& file, const std::string& value)
        {
            const uint64_t size = static_cast<uint64_t>(value.size());
            if (!WritePod(file, size))
                return false;

            if (size > 0)
                file.write(value.data(), static_cast<std::streamsize>(size));

            return file.good();
        }

        bool ReadString(std::ifstream& file, std::string& value)
        {
            uint64_t size = 0;
            if (!ReadPod(file, size))
                return false;

            value.clear();
            if (size == 0)
                return true;

            value.resize(static_cast<size_t>(size));
            file.read(value.data(), static_cast<std::streamsize>(size));
            return file.good();
        }

        bool WriteMagic(std::ofstream& file, const std::array<char, 8>& magic)
        {
            file.write(magic.data(), static_cast<std::streamsize>(magic.size()));
            return file.good();
        }

        bool ReadAndCheckMagic(std::ifstream& file, const std::array<char, 8>& expected)
        {
            std::array<char, 8> actual{};
            file.read(actual.data(), static_cast<std::streamsize>(actual.size()));
            return file.good() && actual == expected;
        }

        bool EnsureParentDirectory(const std::string& path)
        {
            const std::filesystem::path fsPath(path);
            if (!fsPath.has_parent_path())
                return true;

            std::filesystem::create_directories(fsPath.parent_path());
            return true;
        }
    }

    std::string BinaryResourceCache::MeshCachePath(const std::string& sourcePath)
    {
        return sourcePath + ".uwumesh";
    }

    std::string BinaryResourceCache::TextureCachePath(const std::string& sourcePath)
    {
        return sourcePath + ".uwutex";
    }

    bool BinaryResourceCache::IsFresh(const std::string& sourcePath, const std::string& cachePath)
    {
        if (!std::filesystem::exists(sourcePath) || !std::filesystem::exists(cachePath))
            return false;

        return std::filesystem::last_write_time(cachePath) >= std::filesystem::last_write_time(sourcePath);
    }

    bool BinaryResourceCache::LoadMesh(const std::string& cachePath, MeshAsset& out, std::string& error)
    {
        std::ifstream file(cachePath, std::ios::binary);
        if (!file.is_open())
        {
            error = "cannot open mesh binary cache";
            return false;
        }

        if (!ReadAndCheckMagic(file, kMeshMagic))
        {
            error = "invalid mesh binary magic";
            return false;
        }

        uint32_t version = 0;
        if (!ReadPod(file, version) || version != kVersion)
        {
            error = "unsupported mesh binary version";
            return false;
        }

        MeshAsset asset;
        asset.mesh.primitive = PrimitiveType::CustomMesh;

        uint64_t vertexCount = 0;
        uint64_t indexCount = 0;
        if (!ReadPod(file, vertexCount) || !ReadPod(file, indexCount))
        {
            error = "invalid mesh binary counts";
            return false;
        }

        asset.mesh.vertices.resize(static_cast<size_t>(vertexCount));
        asset.mesh.indices.resize(static_cast<size_t>(indexCount));

        if (vertexCount > 0)
        {
            file.read(
                reinterpret_cast<char*>(asset.mesh.vertices.data()),
                static_cast<std::streamsize>(asset.mesh.vertices.size() * sizeof(Vertex)));
        }

        if (indexCount > 0)
        {
            file.read(
                reinterpret_cast<char*>(asset.mesh.indices.data()),
                static_cast<std::streamsize>(asset.mesh.indices.size() * sizeof(uint32_t)));
        }

        if (!file.good())
        {
            error = "mesh binary data is truncated";
            return false;
        }

        if (!ReadPod(file, asset.material.hasDiffuseColor)
            || !ReadPod(file, asset.material.hasDiffuseTexture)
            || !ReadPod(file, asset.material.diffuseColor)
            || !ReadString(file, asset.material.diffuseTexturePath))
        {
            error = "invalid mesh material binary data";
            return false;
        }

        out = std::move(asset);
        return true;
    }

    bool BinaryResourceCache::SaveMesh(const std::string& cachePath, const MeshAsset& asset, std::string& error)
    {
        EnsureParentDirectory(cachePath);

        std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            error = "cannot open mesh binary cache for writing";
            return false;
        }

        const uint64_t vertexCount = static_cast<uint64_t>(asset.mesh.vertices.size());
        const uint64_t indexCount = static_cast<uint64_t>(asset.mesh.indices.size());

        if (!WriteMagic(file, kMeshMagic)
            || !WritePod(file, kVersion)
            || !WritePod(file, vertexCount)
            || !WritePod(file, indexCount))
        {
            error = "cannot write mesh binary header";
            return false;
        }

        if (vertexCount > 0)
        {
            file.write(
                reinterpret_cast<const char*>(asset.mesh.vertices.data()),
                static_cast<std::streamsize>(asset.mesh.vertices.size() * sizeof(Vertex)));
        }

        if (indexCount > 0)
        {
            file.write(
                reinterpret_cast<const char*>(asset.mesh.indices.data()),
                static_cast<std::streamsize>(asset.mesh.indices.size() * sizeof(uint32_t)));
        }

        if (!WritePod(file, asset.material.hasDiffuseColor)
            || !WritePod(file, asset.material.hasDiffuseTexture)
            || !WritePod(file, asset.material.diffuseColor)
            || !WriteString(file, asset.material.diffuseTexturePath)
            || !file.good())
        {
            error = "cannot write mesh binary data";
            return false;
        }

        return true;
    }

    bool BinaryResourceCache::LoadTexture(const std::string& cachePath, TextureAsset& out, std::string& error)
    {
        std::ifstream file(cachePath, std::ios::binary);
        if (!file.is_open())
        {
            error = "cannot open texture binary cache";
            return false;
        }

        if (!ReadAndCheckMagic(file, kTextureMagic))
        {
            error = "invalid texture binary magic";
            return false;
        }

        uint32_t version = 0;
        uint64_t pixelCount = 0;
        TextureAsset asset;

        if (!ReadPod(file, version)
            || version != kVersion
            || !ReadPod(file, asset.texture.width)
            || !ReadPod(file, asset.texture.height)
            || !ReadPod(file, asset.texture.channels)
            || !ReadPod(file, pixelCount))
        {
            error = "invalid texture binary header";
            return false;
        }

        asset.texture.pixels.resize(static_cast<size_t>(pixelCount));
        if (pixelCount > 0)
        {
            file.read(
                reinterpret_cast<char*>(asset.texture.pixels.data()),
                static_cast<std::streamsize>(asset.texture.pixels.size()));
        }

        if (!file.good())
        {
            error = "texture binary data is truncated";
            return false;
        }

        out = std::move(asset);
        return true;
    }

    bool BinaryResourceCache::SaveTexture(const std::string& cachePath, const TextureAsset& asset, std::string& error)
    {
        EnsureParentDirectory(cachePath);

        std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            error = "cannot open texture binary cache for writing";
            return false;
        }

        const uint64_t pixelCount = static_cast<uint64_t>(asset.texture.pixels.size());
        if (!WriteMagic(file, kTextureMagic)
            || !WritePod(file, kVersion)
            || !WritePod(file, asset.texture.width)
            || !WritePod(file, asset.texture.height)
            || !WritePod(file, asset.texture.channels)
            || !WritePod(file, pixelCount))
        {
            error = "cannot write texture binary header";
            return false;
        }

        if (pixelCount > 0)
        {
            file.write(
                reinterpret_cast<const char*>(asset.texture.pixels.data()),
                static_cast<std::streamsize>(asset.texture.pixels.size()));
        }

        if (!file.good())
        {
            error = "cannot write texture binary data";
            return false;
        }

        return true;
    }
}
