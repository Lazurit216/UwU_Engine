#include "uwupch.h"
#include "IRenderer.h"

#ifdef UWU_PLATFORM_WINDOWS
    #include "DirectX12/DX12Renderer.h"
#endif

namespace UwU_Engine
{
    std::unique_ptr<IRenderer> IRenderer::Create()
    {
#ifdef UWU_PLATFORM_WINDOWS
        return std::make_unique<DX12Renderer>();
#else
        UWU_ENGINE_ERROR("No renderer for this platform");
        return nullptr;
#endif
    }
}