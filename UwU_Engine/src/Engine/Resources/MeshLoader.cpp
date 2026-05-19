#include "uwupch.h"
#include "MeshLoader.h"

#ifdef UWU_HAS_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#endif

namespace UwU_Engine
{
    namespace
    {
        struct Vec2
        {
            float x = 0.0f;
            float y = 0.0f;
        };

        struct Vec3
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        std::string Trim(const std::string& value)
        {
            const size_t first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                return {};

            const size_t last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        std::string ReadRestOfLine(std::istringstream& stream)
        {
            std::string rest;
            std::getline(stream, rest);
            return Trim(rest);
        }

        std::string ExtractTexturePath(const std::string& mapLineRest)
        {
            const std::string value = Trim(mapLineRest);
            if (value.empty())
                return {};

            if (value[0] != '-')
                return value;

            std::istringstream stream(value);
            std::vector<std::string> tokens;
            std::string token;
            while (stream >> token)
                tokens.push_back(token);

            return tokens.empty() ? std::string{} : tokens.back();
        }

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

        int ParseObjIndex(const std::string& value)
        {
            if (value.empty())
                return 0;

            return std::stoi(value);
        }

        uint32_t ResolveObjIndex(int rawIndex, size_t count)
        {
            if (rawIndex > 0)
                return static_cast<uint32_t>(rawIndex - 1);

            if (rawIndex < 0)
                return static_cast<uint32_t>(static_cast<int>(count) + rawIndex);

            return UINT32_MAX;
        }

        bool ParseFaceToken(const std::string& token, int& position, int& texcoord, int& normal)
        {
            position = 0;
            texcoord = 0;
            normal = 0;

            std::array<std::string, 3> parts;
            size_t start = 0;
            for (size_t i = 0; i < parts.size(); ++i)
            {
                const size_t slash = token.find('/', start);
                parts[i] = token.substr(start, slash == std::string::npos ? std::string::npos : slash - start);

                if (slash == std::string::npos)
                    break;

                start = slash + 1;
            }

            try
            {
                position = ParseObjIndex(parts[0]);
                texcoord = ParseObjIndex(parts[1]);
                normal = ParseObjIndex(parts[2]);
            }
            catch (const std::exception&)
            {
                return false;
            }

            return position != 0;
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
                vertex.position[2] = (vertex.position[2] - center.z) * scale + 0.5f;
            }
        }

        std::unordered_map<std::string, MeshMaterialData> LoadMtlLibrary(const std::string& mtlPath)
        {
            std::unordered_map<std::string, MeshMaterialData> materials;

            std::ifstream file(mtlPath);
            if (!file.is_open())
            {
                UWU_ENGINE_WARN("[MeshLoader] Cannot open MTL '{}'", mtlPath);
                return materials;
            }

            MeshMaterialData* current = nullptr;
            std::string line;
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;

                std::istringstream stream(line);
                std::string prefix;
                stream >> prefix;

                if (prefix == "newmtl")
                {
                    const std::string name = ReadRestOfLine(stream);
                    if (!name.empty())
                        current = &materials[name];
                }
                else if (current && prefix == "Kd")
                {
                    stream >> current->diffuseColor.r
                        >> current->diffuseColor.g
                        >> current->diffuseColor.b;
                    current->hasDiffuseColor = true;
                }
                else if (current && prefix == "d")
                {
                    stream >> current->diffuseColor.a;
                    current->hasDiffuseColor = true;
                }
                else if (current && prefix == "map_Kd")
                {
                    const std::string texturePath = ExtractTexturePath(ReadRestOfLine(stream));
                    current->diffuseTexturePath = ResolveReferencedPath(mtlPath, texturePath);
                    current->hasDiffuseTexture = !current->diffuseTexturePath.empty();
                }
            }

            UWU_ENGINE_INFO("[MeshLoader] Loaded MTL '{}' ({} materials)", mtlPath, materials.size());
            return materials;
        }

        void ApplyMaterialFromLibraries(
            MeshAsset& asset,
            const std::vector<std::string>& materialLibraries,
            const std::string& preferredMaterialName)
        {
            for (const std::string& libraryPath : materialLibraries)
            {
                const auto materials = LoadMtlLibrary(libraryPath);
                if (materials.empty())
                    continue;

                if (!preferredMaterialName.empty())
                {
                    auto it = materials.find(preferredMaterialName);
                    if (it != materials.end())
                    {
                        asset.material = it->second;
                        return;
                    }
                }

                asset.material = materials.begin()->second;
                return;
            }
        }

        void ApplyObjMaterialReferences(const std::string& objPath, MeshAsset& asset)
        {
            std::ifstream file(objPath);
            if (!file.is_open())
                return;

            std::vector<std::string> materialLibraries;
            std::string activeMaterialName;

            std::string line;
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;

                std::istringstream stream(line);
                std::string prefix;
                stream >> prefix;

                if (prefix == "mtllib")
                {
                    const std::string libraryPath = ReadRestOfLine(stream);
                    if (!libraryPath.empty())
                        materialLibraries.push_back(ResolveReferencedPath(objPath, libraryPath));
                }
                else if (prefix == "usemtl" && activeMaterialName.empty())
                {
                    activeMaterialName = ReadRestOfLine(stream);
                }
            }

            ApplyMaterialFromLibraries(asset, materialLibraries, activeMaterialName);
        }

        bool LoadObjFallback(const std::string& path, MeshAsset& out, std::string& error)
        {
            std::ifstream file(path);
            if (!file.is_open())
            {
                error = "cannot open OBJ file";
                return false;
            }

            std::vector<Vec3> positions;
            std::vector<Vec2> texcoords;
            std::vector<Vec3> normals;
            std::vector<std::string> materialLibraries;
            std::string activeMaterialName;
            std::unordered_map<std::string, uint32_t> vertexCache;

            MeshData mesh;
            mesh.primitive = PrimitiveType::CustomMesh;
            mesh.sourcePath = path;

            std::string line;
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;

                std::istringstream stream(line);
                std::string prefix;
                stream >> prefix;

                if (prefix == "v")
                {
                    Vec3 position;
                    stream >> position.x >> position.y >> position.z;
                    positions.push_back(position);
                }
                else if (prefix == "mtllib")
                {
                    const std::string libraryPath = ReadRestOfLine(stream);
                    if (!libraryPath.empty())
                        materialLibraries.push_back(ResolveReferencedPath(path, libraryPath));
                }
                else if (prefix == "usemtl")
                {
                    if (activeMaterialName.empty())
                        activeMaterialName = ReadRestOfLine(stream);
                }
                else if (prefix == "vt")
                {
                    Vec2 texcoord;
                    stream >> texcoord.x >> texcoord.y;
                    texcoords.push_back(texcoord);
                }
                else if (prefix == "vn")
                {
                    Vec3 normal;
                    stream >> normal.x >> normal.y >> normal.z;
                    normals.push_back(normal);
                }
                else if (prefix == "f")
                {
                    std::vector<uint32_t> faceIndices;
                    std::string token;
                    while (stream >> token)
                    {
                        auto cacheIt = vertexCache.find(token);
                        if (cacheIt != vertexCache.end())
                        {
                            faceIndices.push_back(cacheIt->second);
                            continue;
                        }

                        int rawPosition = 0;
                        int rawTexcoord = 0;
                        int rawNormal = 0;
                        if (!ParseFaceToken(token, rawPosition, rawTexcoord, rawNormal))
                            continue;

                        const uint32_t positionIndex = ResolveObjIndex(rawPosition, positions.size());
                        if (positionIndex >= positions.size())
                            continue;

                        Vertex vertex{};
                        const Vec3& position = positions[positionIndex];
                        vertex.position[0] = position.x;
                        vertex.position[1] = position.y;
                        vertex.position[2] = position.z;

                        const uint32_t texcoordIndex = ResolveObjIndex(rawTexcoord, texcoords.size());
                        if (texcoordIndex < texcoords.size())
                        {
                            vertex.uv[0] = texcoords[texcoordIndex].x;
                            vertex.uv[1] = 1.0f - texcoords[texcoordIndex].y;
                        }

                        const uint32_t normalIndex = ResolveObjIndex(rawNormal, normals.size());
                        if (normalIndex < normals.size())
                        {
                            const Vec3& normal = normals[normalIndex];
                            vertex.normal[0] = normal.x;
                            vertex.normal[1] = normal.y;
                            vertex.normal[2] = normal.z;
                        }

                        vertex.color[0] = 1.0f;
                        vertex.color[1] = 1.0f;
                        vertex.color[2] = 1.0f;

                        const uint32_t newIndex = static_cast<uint32_t>(mesh.vertices.size());
                        mesh.vertices.push_back(vertex);
                        vertexCache.emplace(token, newIndex);
                        faceIndices.push_back(newIndex);
                    }

                    if (faceIndices.size() >= 3)
                    {
                        for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
                        {
                            mesh.indices.push_back(faceIndices[0]);
                            mesh.indices.push_back(faceIndices[i]);
                            mesh.indices.push_back(faceIndices[i + 1]);
                        }
                    }
                }
            }

            if (mesh.vertices.empty() || mesh.indices.empty())
            {
                error = "OBJ file has no renderable triangles";
                return false;
            }

            NormalizeMeshForCurrentRenderer(mesh);
            out.mesh = std::move(mesh);
            ApplyMaterialFromLibraries(out, materialLibraries, activeMaterialName);
            if (out.material.hasDiffuseTexture)
            {
                UWU_ENGINE_INFO("[MeshLoader] OBJ material texture '{}'",
                    out.material.diffuseTexturePath);
            }
            UWU_ENGINE_INFO("[MeshLoader] Loaded OBJ fallback '{}' ({} vertices, {} indices)",
                path, out.mesh.vertices.size(), out.mesh.indices.size());
            return true;
        }

#ifdef UWU_HAS_ASSIMP
        void ApplyAssimpMaterial(const aiScene* scene, unsigned int materialIndex, const std::string& modelPath, MeshAsset& out)
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
                UWU_ENGINE_INFO("[MeshLoader] Assimp material texture '{}'",
                    out.material.diffuseTexturePath);
            }
        }

        bool LoadAssimp(const std::string& path, MeshAsset& out, std::string& error)
        {
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
            unsigned int selectedMaterialIndex = scene->mNumMaterials;
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
            if (!out.material.hasDiffuseTexture)
            {
                ApplyObjMaterialReferences(path, out);
                if (out.material.hasDiffuseTexture)
                {
                    UWU_ENGINE_INFO("[MeshLoader] OBJ material texture fallback '{}'",
                        out.material.diffuseTexturePath);
                }
            }
            UWU_ENGINE_INFO("[MeshLoader] Loaded Assimp mesh '{}' ({} vertices, {} indices)",
                path, out.mesh.vertices.size(), out.mesh.indices.size());
            return true;
        }
#endif
    }

    bool MeshLoader::Load(const std::string& path, MeshAsset& out, std::string& error)
    {
#ifdef UWU_HAS_ASSIMP
        if (LoadAssimp(path, out, error))
            return true;

        UWU_ENGINE_WARN("[MeshLoader] Assimp failed for '{}': {}; trying OBJ fallback",
            path, error);
#endif

        return LoadObjFallback(path, out, error);
    }
}
