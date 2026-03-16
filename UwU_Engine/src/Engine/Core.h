#pragma once

#ifdef UWU_PLATFORM_WINDOWS
	#ifdef UWU_BUILD_DLL
		#define UWU_API __declspec(dllexport)
	#else 
		#define UWU_API __declspec(dllimport)
	#endif
#else 
	#error UwU Engine only supports Windows now!
#endif
