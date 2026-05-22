#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <xmemory>
#include <xutility>

#include "Engine/Logger.h"
//#include "Engine/Window.h"

#ifdef UWU_PLATFORM_WINDOWS
	#define NOMINMAX
	#include <Windows.h>

	// DirectX headers (alphabetical)
	#include <d3d12.h>
	#include <d3d12sdklayers.h>  // ID3D12Debug
	#include <D3Dcompiler.h>
	#include <DirectXCollision.h>
	#include <DirectXColors.h>
	#include <DirectXMath.h>
	#include <DirectXPackedVector.h>
	#include <dxgi1_4.h>
	#include <dxgi1_6.h>
	#include <wrl.h>
	#include <wrl/client.h>
#endif 
