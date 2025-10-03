cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VULKAN_SDK_VERSION "Version of Vulkan SDK to use" "1.4.321.0")

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
fetch_dependency(https://github.com/KhronosGroup/Vulkan-Headers.git vulkan-sdk-${VULKAN_SDK_VERSION} vulkan-headers)
fetch_dependency(https://github.com/KhronosGroup/glslang.git vulkan-sdk-${VULKAN_SDK_VERSION} glslang ENABLE_PCH=OFF GLSLANG_ENABLE_INSTALL=OFF GLSLANG_TESTS=OFF ENABLE_OPT=OFF)

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

add_library(${PROJECT_NAME} ${INC} ${SRC})

# Set Include Directories
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/dep/vulkan-headers-src/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/dep/)

target_link_libraries(${PROJECT_NAME} PUBLIC glslang)
target_link_libraries(${PROJECT_NAME} PUBLIC SPIRV)
