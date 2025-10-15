cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VULKAN_SDK_STATIC_LIB "Link Against Installed Vulkan SDK Statically" OFF)
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
    
    # Apply cache variables with intelligent type detection and FORCE override
    # CRITICAL: Set these BEFORE FetchContent_Declare
    foreach(arg IN LISTS ARGN)
        string(REPLACE "=" ";" kv "${arg}")
        list(GET kv 0 key)
        list(GET kv 1 val)
       
        # Determine the appropriate cache variable type based on value
        if(val MATCHES "^(ON|OFF|TRUE|FALSE)$")
            # Boolean values
            set(${key} ${val} CACHE BOOL "Forced by fetch_dependency for ${ANAME}" FORCE)
        else()
            # Everything else as STRING
            set(${key} ${val} CACHE STRING "Forced by fetch_dependency for ${ANAME}" FORCE)
        endif()
    endforeach()
    
    # Actually fetch it - cache variables should now be available to subdirectory
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

# Check if user wishes to link Vulkan statically
if(VULKAN_SDK_STATIC_LIB)
    # Search for Vulkan package
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        message(STATUS "Geodesy-GPU: Vulkan SDK found.")
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
        message(STATUS "Geodesy-GPU: Vulkan SDK Root Path: ${VULKAN_SDK_ROOT_DIR}")
    endif()
endif()

add_library(${PROJECT_NAME} ${INC} ${SRC})

# Set Include Directories
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)
if(VULKAN_SDK_STATIC_LIB AND Vulkan_FOUND)
    # Use SDK Header Path if Vulkan was found
    target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/)
else()
    # Fallback to fetched Vulkan-Headers if Vulkan not found
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/dep/vulkan-headers-src/include/)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/dep/)

if(VULKAN_SDK_STATIC_LIB AND Vulkan_FOUND)
    # Link against SDK library if found
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARY})
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC glslang)
target_link_libraries(${PROJECT_NAME} PUBLIC SPIRV)
