#include "uwupch.h"
#include "MeshLoader.h"

#include "Engine/Resources/BinaryResourceCache.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace UwU_Engine
{
    namespace
    {
        struct Vec3
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        std::string ResolveReferencedPath(const std::string& ownerFilePath, const std::string& referencedPath)
        {
            if (referencedPath.empty())
                return {};

            std::filesystem::path path(referencedPath);
            if (path.is_absolute())
                return path.lexically_normal().string();

            const std::filesystem::path ownerPath(ownerFilePath);
            return (ownerPath.parent_path() / path).lexically_normal().string();
        }

        std::string ResolveCachedTexturePath(const std::string& modelPath, const std::string& cachedTexturePath)
        {
            if (cachedTexturePath.empty())
                return {};

            std::filesystem::path path(cachedTexturePath);
            if (std::filesystem::exists(path))
                return path.lexically_normal().string();

            if (path.is_absolute())
                return path.lexically_normal().string();

            const std::string modelRelative = ResolveReferencedPath(modelPath, cachedTexturePath);
            if (std::filesystem::exists(modelRelative))
                return modelRelative;

            const std::filesystem::path modelFile(modelPath);
            const std::filesystem::path assetsRoot = modelFile.parent_path().parent_path().parent_path();
            const std::filesystem::path textureByName = assetsRoot / "Textures" / path.filename();
            if (std::filesystem::exists(textureByName))
                return textureByName.lexically_normal().string();

            return modelRelative;
        }

        void NormalizeMeshForCurrentRenderer(MeshData& mesh)
        {
            if (mesh.vertices.empty())
                return;

            Vec3 minValue{
                mesh.vertices[0].position[0],
                mesh.vertices[0].position[1],
                mesh.vertices[0].position[2]
            };
            Vec3 maxValue = minValue;

            for (const Vertex& vertex : mesh.vertices)
            {
                minValue.x = (std::min)(minValue.x, vertex.position[0]);
                minValue.y = (std::min)(minValue.y, vertex.position[1]);
                minValue.z = (std::min)(minValue.z, vertex.position[2]);
                maxValue.x = (std::max)(maxValue.x, vertex.position[0]);
                maxValue.y = (std::max)(maxValue.y, vertex.position[1]);
                maxValue.z = (std::max)(maxValue.z, vertex.position[2]);
            }

            const Vec3 center{
                (minValue.x + maxValue.x) * 0.5f,
                (minValue.y + maxValue.y) * 0.5f,
                (minValue.z + maxValue.z) * 0.5f
            };
            const float extentX = maxValue.x - minValue.x;
            const float extentY = maxValue.y - minValue.y;
            const float extentZ = maxValue.z - minValue.z;
            const float largestExtent = (std::max)(extentX, (std::max)(extentY, extentZ));

            if (largestExtent <= 0.0f)
                return;

            const float scale = 1.0f / largestExtent;
            for (Vertex& vertex : mesh.vertices)
            {
                vertex.position[0] = (vertex.position[0] - center.x) * scale;
                vertex.position[1] = (vertex.position[1] - center.y) * scale;
                vertex.position[2] = (vertex.position[2] - center.z) * scale;
            }
        }

        void ApplyAssimpMaterial(
            const aiScene* scene,
            unsigned int materialIndex,
            const std::string& modelPath,
            MeshAsset& out)
        {
            if (!scene || materialIndex >= scene->mNumMaterials)
                return;

            const aiMaterial* material = scene->mMaterials[materialIndex];
            if (!material)
                return;

            aiColor4D diffuseColor;
            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor))
            {
                out.material.diffuseColor = Color4{
                    diffuseColor.r,
                    diffuseColor.g,
                    diffuseColor.b,
                    diffuseColor.a
                };
                out.material.hasDiffuseColor = true;
            }

            aiString diffuseTexturePath;
            if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexturePath))
            {
                out.material.diffuseTexturePath = ResolveReferencedPath(modelPath, diffuseTexturePath.C_Str());
                out.material.hasDiffuseTexture = !out.material.diffuseTexturePath.empty();
                UWU_ENGINE_INFO("[MeshLoader] Material texture '{}'", out.material.diffuseTexturePath);
            }
        }
    }

    bool MeshLoader::Load(const std::string& path, MeshAsset& out, std::string& error)
    {
        const std::string cachePath = BinaryResourceCache::MeshCachePath(path);
        bool cacheIsFresh = BinaryResourceCache::IsFresh(path, cachePath);

        std::filesystem::path companionMaterialPath(path);
        companionMaterialPath.replace_extension(".mtl");
        if (cacheIsFresh
            && std::filesystem::exists(companionMaterialPath)
            && std::filesystem::last_write_time(cachePath) < std::filesystem::last_write_time(companionMaterialPath))
        {
            cacheIsFresh = false;
        }

        if (cacheIsFresh)
        {
            std::string cacheError;
            if (BinaryResourceCache::LoadMesh(cachePath, out, cacheError))
            {
                out.mesh.sourcePath = path;
                if (out.material.hasDiffuseTexture)
                    out.material.diffuseTexturePath = ResolveCachedTexturePath(path, out.material.diffuseTexturePath);
                UWU_ENGINE_INFO("[MeshLoader] Loaded mesh binary cache '{}'", cachePath);
                return true;
            }

            UWU_ENGINE_WARN("[MeshLoader] Mesh binary cache '{}' failed: {}", cachePath, cacheError);
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate
            | aiProcess_GenSmoothNormals
            | aiProcess_JoinIdenticalVertices
            | aiProcess_ImproveCacheLocality
            | aiProcess_FlipUVs);

        if (!scene || !scene->HasMeshes())
        {
            error = importer.GetErrorString();
            if (error.empty())
                error = "file contains no meshes";
            return false;
        }

        MeshData mesh;
        mesh.primitive = PrimitiveType::CustomMesh;
        mesh.sourcePath = path;

        uint32_t vertexOffset = 0;
        unsigned int selectedMaterialIndex = UINT_MAX;

        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh* sourceMesh = scene->mMeshes[meshIndex];
            if (!sourceMesh || !sourceMesh->HasPositions())
                continue;

            if (selectedMaterialIndex == UINT_MAX)
                selectedMaterialIndex = sourceMesh->mMaterialIndex;

            mesh.vertices.reserve(mesh.vertices.size() + sourceMesh->mNumVertices);
            for (unsigned int i = 0; i < sourceMesh->mNumVertices; ++i)
            {
                Vertex vertex{};
                vertex.position[0] = sourceMesh->mVertices[i].x;
                vertex.position[1] = sourceMesh->mVertices[i].y;
                vertex.position[2] = sourceMesh->mVertices[i].z;

                if (sourceMesh->HasNormals())
                {
                    vertex.normal[0] = sourceMesh->mNormals[i].x;
                    vertex.normal[1] = sourceMesh->mNormals[i].y;
                    vertex.normal[2] = sourceMesh->mNormals[i].z;
                }

                if (sourceMesh->HasTextureCoords(0))
                {
                    vertex.uv[0] = sourceMesh->mTextureCoords[0][i].x;
                    vertex.uv[1] = sourceMesh->mTextureCoords[0][i].y;
                }

                vertex.color[0] = 1.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = 1.0f;
                mesh.vertices.push_back(vertex);
            }

            for (unsigned int faceIndex = 0; faceIndex < sourceMesh->mNumFaces; ++faceIndex)
            {
                const aiFace& face = sourceMesh->mFaces[faceIndex];
                if (face.mNumIndices != 3)
                    continue;

                mesh.indices.push_back(vertexOffset + face.mIndices[0]);
                mesh.indices.push_back(vertexOffset + face.mIndices[1]);
                mesh.indices.push_back(vertexOffset + face.mIndices[2]);
            }

            vertexOffset += sourceMesh->mNumVertices;
        }

        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            error = "loaded scene has no renderable triangles";
            return false;
        }

        NormalizeMeshForCurrentRenderer(mesh);
        out.mesh = std::move(mesh);
        ApplyAssimpMaterial(scene, selectedMaterialIndex, path, out);

        std::string cacheError;
        if (!BinaryResourceCache::SaveMesh(cachePath, out, cacheError))
            UWU_ENGINE_WARN("[MeshLoader] Cannot save mesh binary cache '{}': {}", cachePath, cacheError);

        UWU_ENGINE_INFO("[MeshLoader] Loaded mesh '{}' ({} vertices, {} indices)",
            path, out.mesh.vertices.size(), out.mesh.indices.size());
        return true;
    }
}
