project(UwU_Engine)

file(GLOB_RECURSE ENGINE_SOURCES     
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

message(STATUS "ENGINE_SOURCES list  = ${ENGINE_SOURCES}")

add_library(UwU_Engine SHARED ${ENGINE_SOURCES})

target_include_directories(UwU_Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(UwU_Engine PRIVATE UWU_BUILD_DLL UWU_PLATFORM_WINDOWS)


# Create folder and copy DLL next to Sandbox.exe
add_custom_command(TARGET UwU_Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/Sandbox
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:UwU_Engine>
        ${OUTPUT_DIR}/Sandbox/
    COMMENT "Copying UwU_Engine.dll to bin/${CMAKE_BUILD_TYPE}-${PLATFORM}/Sandbox/"
)