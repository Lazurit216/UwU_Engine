project(UwU_Engine)

file(GLOB_RECURSE ENGINE_SOURCES     
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hlsl")
foreach(SHADER ${SHADER_FILES})
    set_source_files_properties(${SHADER} PROPERTIES
        VS_TOOL_OVERRIDE "None"   # tells MSBuild: display only, no compile step
    )
endforeach()
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Shaders" FILES ${SHADER_FILES})

file(GLOB_RECURSE JSON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../Sandbox/*.json")
foreach(JSON ${JSON_FILES})
    set_source_files_properties(${JSON} PROPERTIES
        VS_TOOL_OVERRIDE "None"
    )
endforeach()
source_group("Config" FILES ${JSON_FILES})

message(STATUS "ENGINE_SOURCES list  = ${ENGINE_SOURCES}")
message(STATUS "SHADERS list  = ${SHADER_FILES}")
message(STATUS "CONFIGS list  = ${JSON_FILES}")

add_library(UwU_Engine SHARED ${ENGINE_SOURCES} ${SHADER_FILES} ${JSON_FILES})

set_target_properties(UwU_Engine PROPERTIES
    FOLDER "UwU Engine" 
    RUNTIME_OUTPUT_DIRECTORY   "${OUTPUT_DIR}/UwU_Engine"
    LIBRARY_OUTPUT_DIRECTORY   "${OUTPUT_DIR}/UwU_Engine"
    ARCHIVE_OUTPUT_DIRECTORY   "${OUTPUT_DIR}/UwU_Engine"
    PDB_OUTPUT_DIRECTORY       "${INT_DIR}/UwU_Engine"
    COMPILE_PDB_OUTPUT_DIRECTORY "${INT_DIR}/UwU_Engine"
)

foreach(SOURCE_FILE ${ENGINE_SOURCES})
    # Get path relative to src/
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src ${SOURCE_FILE})
    
    # Get folder name, e.g. "Core/Application.cpp" -> "Core"
    get_filename_component(SOURCE_DIR ${REL_PATH} DIRECTORY)
    
    if(SOURCE_DIR)
        # Create a filter named after the folder
        source_group("src\\${SOURCE_DIR}" FILES ${SOURCE_FILE})
    else()
        # Files in the src root
        source_group("src" FILES ${SOURCE_FILE})
    endif()
endforeach()

target_include_directories(UwU_Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(UwU_Engine PRIVATE UWU_BUILD_DLL UWU_PLATFORM_WINDOWS)

set(UWU_EXTERNAL_LIBS_DIR "${CMAKE_SOURCE_DIR}/ExternalLibs")

set(UWU_NLOHMANN_JSON_INCLUDE_DIR "${UWU_EXTERNAL_LIBS_DIR}/nlohmann_json/include")
if(EXISTS "${UWU_NLOHMANN_JSON_INCLUDE_DIR}/nlohmann/json.hpp")
    target_include_directories(UwU_Engine PRIVATE "${UWU_NLOHMANN_JSON_INCLUDE_DIR}")
    message(STATUS "nlohmann_json enabled from ${UWU_NLOHMANN_JSON_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "nlohmann_json headers not found in ${UWU_NLOHMANN_JSON_INCLUDE_DIR}")
endif()

set(UWU_STB_IMAGE_DIR "${UWU_EXTERNAL_LIBS_DIR}/stb_image")
if(EXISTS "${UWU_STB_IMAGE_DIR}/stb_image.h")
    target_include_directories(UwU_Engine PRIVATE "${UWU_STB_IMAGE_DIR}")
    target_compile_definitions(UwU_Engine PRIVATE UWU_HAS_STB_IMAGE=1)
    message(STATUS "stb_image enabled from ${UWU_STB_IMAGE_DIR}")
else()
    message(STATUS "stb_image headers not found in ${UWU_STB_IMAGE_DIR}")
endif()

set(UWU_ASSIMP_ROOT "${UWU_EXTERNAL_LIBS_DIR}/Assimp")
set(UWU_ASSIMP_INCLUDE_DIR "${UWU_ASSIMP_ROOT}/include")
set(UWU_ASSIMP_DEBUG_LIB "${UWU_ASSIMP_ROOT}/lib/Debug/assimp-vc143-mtd.lib")
set(UWU_ASSIMP_DEBUG_DLL "${UWU_ASSIMP_ROOT}/bin/Debug/assimp-vc143-mtd.dll")
set(UWU_ASSIMP_RELEASE_LIB "${UWU_ASSIMP_ROOT}/lib/Release/assimp-vc143-mt.lib")
set(UWU_ASSIMP_RELEASE_DLL "${UWU_ASSIMP_ROOT}/bin/Release/assimp-vc143-mt.dll")
set(UWU_ASSIMP_DEBUG_ENABLED FALSE)
set(UWU_ASSIMP_RELEASE_ENABLED FALSE)

set(UWU_ASSIMP_HEADERS_READY FALSE)
if(EXISTS "${UWU_ASSIMP_INCLUDE_DIR}/assimp/Importer.hpp"
    AND EXISTS "${UWU_ASSIMP_INCLUDE_DIR}/assimp/scene.h"
    AND EXISTS "${UWU_ASSIMP_INCLUDE_DIR}/assimp/postprocess.h")
    set(UWU_ASSIMP_HEADERS_READY TRUE)
endif()

if(UWU_ASSIMP_HEADERS_READY)
    target_include_directories(UwU_Engine PRIVATE "${UWU_ASSIMP_INCLUDE_DIR}")

    if(EXISTS "${UWU_ASSIMP_DEBUG_LIB}")
        target_link_libraries(UwU_Engine PRIVATE "$<$<CONFIG:Debug>:${UWU_ASSIMP_DEBUG_LIB}>")
        target_compile_definitions(UwU_Engine PRIVATE "$<$<CONFIG:Debug>:UWU_HAS_ASSIMP=1>")
        set(UWU_ASSIMP_DEBUG_ENABLED TRUE)
        message(STATUS "Assimp Debug lib enabled from ${UWU_ASSIMP_DEBUG_LIB}")
    endif()

    if(EXISTS "${UWU_ASSIMP_RELEASE_LIB}")
        target_link_libraries(UwU_Engine PRIVATE "$<$<NOT:$<CONFIG:Debug>>:${UWU_ASSIMP_RELEASE_LIB}>")
        target_compile_definitions(UwU_Engine PRIVATE "$<$<NOT:$<CONFIG:Debug>>:UWU_HAS_ASSIMP=1>")
        set(UWU_ASSIMP_RELEASE_ENABLED TRUE)
        message(STATUS "Assimp Release lib enabled from ${UWU_ASSIMP_RELEASE_LIB}")
    endif()
else()
    message(STATUS "Assimp headers are incomplete in ${UWU_ASSIMP_INCLUDE_DIR}; MeshLoader will use OBJ fallback")
endif()

target_precompile_headers(UwU_Engine PUBLIC
    "src/uwupch.h" 
)

# Create folder and copy DLL next to Sandbox.exe
add_custom_command(TARGET UwU_Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/Sandbox
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:UwU_Engine>
        ${OUTPUT_DIR}/Sandbox/
    COMMENT "Copying UwU_Engine.dll to {OUTPUT_DIR}/Sandbox/"
)

if(UWU_ASSIMP_DEBUG_ENABLED AND EXISTS "${UWU_ASSIMP_DEBUG_DLL}")
    add_custom_command(TARGET UwU_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${UWU_ASSIMP_DEBUG_DLL}"
            "${OUTPUT_DIR}/Sandbox/"
        COMMENT "Copying Assimp Debug DLL to Sandbox output"
    )
endif()

if(UWU_ASSIMP_RELEASE_ENABLED AND EXISTS "${UWU_ASSIMP_RELEASE_DLL}")
    add_custom_command(TARGET UwU_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${UWU_ASSIMP_RELEASE_DLL}"
            "${OUTPUT_DIR}/Sandbox/"
        COMMENT "Copying Assimp Release DLL to Sandbox output"
    )
endif()
