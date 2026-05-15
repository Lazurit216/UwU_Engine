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
