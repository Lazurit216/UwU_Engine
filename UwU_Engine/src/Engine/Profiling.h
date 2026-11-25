#pragma once

#ifdef TRACY_ENABLE
    #include <tracy/Tracy.hpp>

    #define UWU_PROFILE_ZONE(name) ZoneScopedN(name)
    #define UWU_PROFILE_FRAME() FrameMark
    #define UWU_PROFILE_PLOT(name, value) TracyPlot(name, value)
    #define UWU_PROFILE_MESSAGE(message) TracyMessageL(message)
#else
    #define UWU_PROFILE_ZONE(name) ((void)0)
    #define UWU_PROFILE_FRAME() ((void)0)
    #define UWU_PROFILE_PLOT(name, value) ((void)0)
    #define UWU_PROFILE_MESSAGE(message) ((void)0)
#endif
