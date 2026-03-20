project(Sandbox)

file(GLOB_RECURSE SANDBOX_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

message(STATUS "SANDBOX_SOURCES list  = ${SANDBOX_SOURCES}")

add_executable(Sandbox ${SANDBOX_SOURCES})

target_include_directories(Sandbox PRIVATE ${CMAKE_SOURCE_DIR}/UwU_Engine/src)

target_link_libraries(Sandbox PRIVATE UwU_Engine)

target_compile_definitions(Sandbox PRIVATE UWU_PLATFORM_WINDOWS)

# Ensure Sandbox output folder exists
add_custom_command(TARGET Sandbox PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/Sandbox
)