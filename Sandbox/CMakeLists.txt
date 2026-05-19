project(Sandbox)

file(GLOB_RECURSE SANDBOX_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE SANDBOX_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hlsl")
foreach(SHADER ${SANDBOX_SHADERS})
    set_source_files_properties(${SHADER} PROPERTIES
        VS_TOOL_OVERRIDE "None"
    )
endforeach()
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Shaders" FILES ${SANDBOX_SHADERS})

file(GLOB_RECURSE SANDBOX_JSON "${CMAKE_CURRENT_SOURCE_DIR}/src/*.json")
foreach(JSON ${SANDBOX_JSON})
    set_source_files_properties(${JSON} PROPERTIES
        VS_TOOL_OVERRIDE "None"
    )
endforeach()
source_group("Config" FILES ${SANDBOX_JSON})

add_executable(Sandbox ${SANDBOX_SOURCES} ${SANDBOX_SHADERS} ${SANDBOX_JSON})

set_target_properties(Sandbox PROPERTIES
    FOLDER "Sandbox"
    RUNTIME_OUTPUT_DIRECTORY   "${OUTPUT_DIR}/Sandbox"
    PDB_OUTPUT_DIRECTORY       "${INT_DIR}/Sandbox"
    COMPILE_PDB_OUTPUT_DIRECTORY "${INT_DIR}/Sandbox"
)

foreach(SOURCE_FILE ${SANDBOX_SOURCES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src ${SOURCE_FILE})
    get_filename_component(SOURCE_DIR ${REL_PATH} DIRECTORY)
    
    if(SOURCE_DIR)
        source_group("src\\${SOURCE_DIR}" FILES ${SOURCE_FILE})
    else()
        source_group("src" FILES ${SOURCE_FILE})
    endif()
endforeach()

target_include_directories(Sandbox PRIVATE ${CMAKE_SOURCE_DIR}/UwU_Engine/src)
target_link_libraries(Sandbox PRIVATE UwU_Engine)
target_compile_definitions(Sandbox PRIVATE UWU_PLATFORM_WINDOWS)

#Copy shaders to output (runtime compilation via D3DCompileFromFile)
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${OUTPUT_DIR}/Sandbox/Assets/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Assets/Shaders"
        "${OUTPUT_DIR}/Sandbox/Assets/Shaders"
    COMMENT "Copying shaders to Sandbox output"
)

#Copy engine_config.json next to Sandbox.exe
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/engine_config.json"
        "$<TARGET_FILE_DIR:Sandbox>/engine_config.json"
    COMMENT "Copying engine_config.json to Sandbox output"
)

#Copy scene assets to Assets/Scenes, Unity-style
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${OUTPUT_DIR}/Sandbox/Assets/Scenes"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Assets/Scenes"
        "${OUTPUT_DIR}/Sandbox/Assets/Scenes"
    COMMENT "Copying scenes to Sandbox output"
)
