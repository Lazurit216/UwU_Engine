#pragma once

#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
#include <mutex>
#include <format>
#include <cstdint>
#include <optional>
#include <functional>

#include "Engine/Logger.h"
//#include "Engine/Window.h"

#ifdef UWU_PLATFORM_WINDOWS
	#include <Windows.h>
#endif 
