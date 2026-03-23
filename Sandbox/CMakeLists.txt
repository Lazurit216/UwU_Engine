project(Sandbox)

file(GLOB_RECURSE SANDBOX_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

#message(STATUS "SANDBOX_SOURCES list  = ${SANDBOX_SOURCES}")

add_executable(Sandbox ${SANDBOX_SOURCES})

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
