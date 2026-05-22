#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Components.h"
#include "Engine/Resources/ResourceManager.h"

namespace UwU_Engine
{
    class RenderResourceBinder
    {
    public:
        bool Update(EntityId entity, MeshRendererComponent& mesh) const
        {
            UpdateMaterial(entity, mesh);
            UpdateMesh(entity, mesh);
            EnsurePlaceholderMesh(mesh);
            UpdateTexture(entity, mesh);
            UpdateShader(entity, mesh);
            return IsShaderReady(mesh);
        }

    private:
        void UpdateMaterial(EntityId entity, MeshRendererComponent& mesh) const
        {
            if (mesh.materialResourcePath.empty())
                return;

            auto& resources = ResourceManager::Instance();
            if (!mesh.materialResource)
                mesh.materialResource = resources.LoadAsync<MaterialAsset>(mesh.materialResourcePath);

            if (!mesh.materialResource)
                return;

            if (mesh.materialResource->IsLoaded() && !mesh.materialResourceApplied)
            {
                ApplyMaterialAsset(mesh, mesh.materialResource->GetData());
                mesh.materialResourceApplied = true;
                mesh.materialLoadFailedLogged = false;
                UWU_ENGINE_INFO("[RenderResourceBinder] Entity {} uses material resource '{}'",
                    entity, mesh.materialResourcePath);
                return;
            }

            if (mesh.materialResource->IsFailed() && !mesh.materialLoadFailedLogged)
            {
                UWU_ENGINE_WARN("[RenderResourceBinder] Entity {} material resource '{}' is unavailable: {}",
                    entity, mesh.materialResourcePath, mesh.materialResource->GetError());
                mesh.materialLoadFailedLogged = true;
            }
        }

        void UpdateMesh(EntityId entity, MeshRendererComponent& mesh) const
        {
            if (mesh.meshResourcePath.empty())
                return;

            auto& resources = ResourceManager::Instance();
            if (!mesh.meshResource)
                mesh.meshResource = resources.LoadAsync<MeshAsset>(mesh.meshResourcePath);

            if (!mesh.meshResource)
                return;

            if (mesh.meshResource->IsLoaded() && !mesh.meshResourceApplied)
            {
                ApplyMeshAsset(mesh, mesh.meshResource->GetData());
                mesh.meshResourceApplied = true;
                mesh.placeholderMeshApplied = false;
                mesh.meshLoadFailedLogged = false;
                ResetDrawable(mesh);
                UWU_ENGINE_INFO("[RenderResourceBinder] Entity {} uses mesh resource '{}'",
                    entity, mesh.meshResourcePath);
                return;
            }

            if (mesh.meshResource->IsFailed() && !mesh.meshLoadFailedLogged)
            {
                UWU_ENGINE_WARN("[RenderResourceBinder] Entity {} mesh resource '{}' is unavailable: {}",
                    entity, mesh.meshResourcePath, mesh.meshResource->GetError());
                mesh.meshLoadFailedLogged = true;
            }
        }

        void UpdateTexture(EntityId entity, MeshRendererComponent& mesh) const
        {
            if (mesh.material.texturePath.empty())
                return;

            auto& resources = ResourceManager::Instance();
            if (!mesh.textureResource)
                mesh.textureResource = resources.LoadAsync<TextureAsset>(mesh.material.texturePath);

            if (!mesh.textureResource)
                return;

            if (mesh.textureResource->IsLoaded() && !mesh.textureResourceApplied)
            {
                const auto& texture = mesh.textureResource->GetData().texture;
                mesh.material.textureWidth = texture.width;
                mesh.material.textureHeight = texture.height;
                mesh.material.textureChannels = texture.channels;
                mesh.material.texturePixels = texture.pixels;
                mesh.textureResourceApplied = true;
                mesh.textureLoadFailedLogged = false;
                ResetDrawable(mesh);
                UWU_ENGINE_INFO("[RenderResourceBinder] Entity {} uses texture resource '{}'",
                    entity, mesh.material.texturePath);
                return;
            }

            if (mesh.textureResource->IsFailed() && !mesh.textureLoadFailedLogged)
            {
                UWU_ENGINE_WARN("[RenderResourceBinder] Entity {} texture resource '{}' is unavailable: {}",
                    entity, mesh.material.texturePath, mesh.textureResource->GetError());
                mesh.textureLoadFailedLogged = true;
            }
        }

        void UpdateShader(EntityId entity, MeshRendererComponent& mesh) const
        {
            const std::string shaderPath = NarrowPath(mesh.material.shaderPath);
            if (shaderPath.empty())
                return;

            auto& resources = ResourceManager::Instance();
            if (!mesh.shaderResource)
                mesh.shaderResource = resources.LoadAsync<ShaderAsset>(shaderPath);

            if (!mesh.shaderResource)
                return;

            if (mesh.shaderResource->IsLoaded() && !mesh.shaderResourceApplied)
            {
                const auto& shader = mesh.shaderResource->GetData().shader;
                mesh.material.shaderSourceCode = shader.sourceCode;
                mesh.material.vertexEntry = shader.vertexEntry;
                mesh.material.pixelEntry = shader.pixelEntry;
                mesh.material.vertexProfile = shader.vertexProfile;
                mesh.material.pixelProfile = shader.pixelProfile;
                mesh.shaderResourceApplied = true;
                mesh.shaderLoadFailedLogged = false;
                ResetDrawable(mesh);
                UWU_ENGINE_INFO("[RenderResourceBinder] Entity {} uses shader resource '{}'",
                    entity, shaderPath);
                return;
            }

            if (mesh.shaderResource->IsFailed() && !mesh.shaderLoadFailedLogged)
            {
                UWU_ENGINE_WARN("[RenderResourceBinder] Entity {} shader resource '{}' is unavailable: {}",
                    entity, shaderPath, mesh.shaderResource->GetError());
                mesh.shaderLoadFailedLogged = true;
            }
        }

        bool IsShaderReady(const MeshRendererComponent& mesh) const
        {
            const std::string shaderPath = NarrowPath(mesh.material.shaderPath);
            if (shaderPath.empty())
                return true;

            return !mesh.material.shaderSourceCode.empty()
                || (mesh.shaderResource && mesh.shaderResource->IsLoaded() && mesh.shaderResourceApplied);
        }

        void EnsurePlaceholderMesh(MeshRendererComponent& mesh) const
        {
            if (mesh.meshResourcePath.empty()
                || mesh.placeholderMeshApplied
                || !mesh.mesh.vertices.empty())
                return;

            mesh.mesh = MeshFactory::CreateTriangle(0.35f, Color4{ 0.70f, 0.70f, 0.72f, 1.0f });
            mesh.placeholderMeshApplied = true;
            ResetDrawable(mesh);
        }

        void ApplyMaterialAsset(MeshRendererComponent& mesh, const MaterialAsset& asset) const
        {
            if (asset.hasBaseColor)
                mesh.material.baseColor = asset.material.baseColor;

            if (asset.hasShaderPath)
                SetShaderPath(mesh, asset.material.shaderPath);

            if (asset.hasTexturePath)
                SetTexturePath(mesh, asset.material.texturePath);

            const bool shaderSettingsChanged =
                mesh.material.vertexEntry != asset.material.vertexEntry
                || mesh.material.pixelEntry != asset.material.pixelEntry
                || mesh.material.vertexProfile != asset.material.vertexProfile
                || mesh.material.pixelProfile != asset.material.pixelProfile;

            mesh.material.vertexEntry = asset.material.vertexEntry;
            mesh.material.pixelEntry = asset.material.pixelEntry;
            mesh.material.vertexProfile = asset.material.vertexProfile;
            mesh.material.pixelProfile = asset.material.pixelProfile;

            if (shaderSettingsChanged)
            {
                mesh.shaderResourceApplied = false;
                ResetDrawable(mesh);
            }

            if (mesh.meshResource && mesh.meshResource->IsLoaded())
                mesh.meshResourceApplied = false;
        }

        void ApplyMeshAsset(MeshRendererComponent& mesh, const MeshAsset& asset) const
        {
            mesh.mesh = asset.mesh;

            if (mesh.material.texturePath.empty() && asset.material.hasDiffuseTexture)
            {
                SetTexturePath(mesh, asset.material.diffuseTexturePath);
                UWU_ENGINE_INFO("[RenderResourceBinder] Using mesh material texture '{}'",
                    mesh.material.texturePath);
            }

            ApplyMaterialColor(mesh.mesh, GetEffectiveColor(mesh.material.baseColor, asset.material));
        }

        Color4 GetEffectiveColor(const Color4& baseColor, const MeshMaterialData& meshMaterial) const
        {
            if (!meshMaterial.hasDiffuseColor)
                return baseColor;

            return Color4{
                baseColor.r * meshMaterial.diffuseColor.r,
                baseColor.g * meshMaterial.diffuseColor.g,
                baseColor.b * meshMaterial.diffuseColor.b,
                baseColor.a * meshMaterial.diffuseColor.a
            };
        }

        void ApplyMaterialColor(MeshData& mesh, const Color4& color) const
        {
            for (Vertex& vertex : mesh.vertices)
            {
                vertex.color[0] *= color.r;
                vertex.color[1] *= color.g;
                vertex.color[2] *= color.b;
            }
        }

        void SetTexturePath(MeshRendererComponent& mesh, const std::string& texturePath) const
        {
            if (mesh.material.texturePath == texturePath)
                return;

            mesh.material.texturePath = texturePath;
            mesh.material.textureWidth = 0;
            mesh.material.textureHeight = 0;
            mesh.material.textureChannels = 0;
            mesh.material.texturePixels.clear();
            mesh.textureResource.reset();
            mesh.textureResourceApplied = false;
            mesh.textureLoadFailedLogged = false;
            ResetDrawable(mesh);
        }

        void SetShaderPath(MeshRendererComponent& mesh, const std::wstring& shaderPath) const
        {
            if (mesh.material.shaderPath == shaderPath)
                return;

            mesh.material.shaderPath = shaderPath;
            mesh.material.shaderSourceCode.clear();
            mesh.shaderResource.reset();
            mesh.shaderResourceApplied = false;
            mesh.shaderLoadFailedLogged = false;
            ResetDrawable(mesh);
        }

        void ResetDrawable(MeshRendererComponent& mesh) const
        {
            if (!mesh.drawable)
                return;

            mesh.drawable->Shutdown();
            mesh.drawable.reset();
            mesh.initFailedLogged = false;
        }

        std::string NarrowPath(const std::wstring& value) const
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t ch : value)
                result.push_back(ch >= 0 && ch <= 127 ? static_cast<char>(ch) : '?');
            return result;
        }
    };
}
