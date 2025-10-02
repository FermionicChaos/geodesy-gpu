cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Utility function for fetching dependencies in one line.
function(fetch_dependency AGIT_REPO AGIT_TAG ANAME)
    # Remaining args are variable=value pairs for cache options
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(FETCHDEP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Declare the dependency
    FetchContent_Declare(
        ${ANAME}
        GIT_REPOSITORY ${AGIT_REPO}
        GIT_TAG        ${AGIT_TAG}
        SOURCE_DIR     ${CMAKE_SOURCE_DIR}/dep/${ANAME}-src
    )

    # Apply cache variables
    foreach(arg IN LISTS ARGN)
        string(REPLACE "=" ";" kv "${arg}")
        list(GET kv 0 key)
        list(GET kv 1 val)
        set(${key} ${val} CACHE BOOL "")
    endforeach()

    # Actually fetch it
    FetchContent_MakeAvailable(${ANAME})
endfunction()

include(FetchContent)

# Fetch Dependencies
fetch_dependency(https://github.com/KhronosGroup/glslang.git vulkan-sdk-1.4.321.0 glslang ENABLE_PCH=OFF GLSLANG_ENABLE_INSTALL=OFF GLSLANG_TESTS=OFF ENABLE_OPT=OFF)

set(PROJECT_NAME "geodesy-gpu")

file(GLOB_RECURSE INC
    "inc/*.h"
)

file(GLOB_RECURSE SRC
    "src/*.h"
    "src/*.cpp"
    "src/*.c"
)

project(${PROJECT_NAME})

find_package(Vulkan REQUIRED)

# Check if Vulkan found and extract SDK root path
if(Vulkan_FOUND)
    message(STATUS "Geodesy: Vulkan found.")
    # Find the last occurrence of "/Include" in the path
    string(FIND "${Vulkan_INCLUDE_DIRS}" "/Include" REMOVE_INDEX REVERSE)

    if (REMOVE_INDEX GREATER -1)
        # Remove the "/Include" part from the path
        string(SUBSTRING "${Vulkan_INCLUDE_DIRS}" 0 ${REMOVE_INDEX} VULKAN_SDK_ROOT_DIR)
    else ()
        # If "/Include" is not found, keep the original path
        set(VULKAN_SDK_ROOT_DIR "${Vulkan_INCLUDE_DIRS}")
    endif ()

    # Print the modified path
    message(STATUS "Geodesy: Vulkan SDK Root Path: ${VULKAN_SDK_ROOT_DIR}")
endif()

add_library(${PROJECT_NAME} ${INC} ${SRC})

# Set Include Directories
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/dep/)

# Set Link Libraries
if(Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARY})
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC glslang)
target_link_libraries(${PROJECT_NAME} PUBLIC SPIRV)
