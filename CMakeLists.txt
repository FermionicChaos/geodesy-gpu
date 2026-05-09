cmake_minimum_required(VERSION 3.20)

# Repository metadata
set(REPO_NAME "geodesy-gpu")
set(REPO_VERSION_MAJOR 0)
set(REPO_VERSION_MINOR 0)
set(REPO_VERSION_PATCH 0)
set(REPO_BRANCH "unknown")
set(REPO_UNIT_TEST_NAME "${REPO_NAME}-unit-test")

# ==============================================================================
# Git Metadata
# ==============================================================================

find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE REPO_BRANCH_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(NOT REPO_BRANCH_RESULT STREQUAL ""
       AND NOT REPO_BRANCH_RESULT STREQUAL "HEAD")
        set(REPO_BRANCH "${REPO_BRANCH_RESULT}")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --exact-match --tags HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE REPO_GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE REPO_GIT_TAG_RESULT
    )

    if(REPO_GIT_TAG_RESULT EQUAL 0)
        string(REGEX REPLACE "^v" "" REPO_GIT_TAG "${REPO_GIT_TAG}")
        if(REPO_GIT_TAG MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
            set(REPO_VERSION_MAJOR "${CMAKE_MATCH_1}")
            set(REPO_VERSION_MINOR "${CMAKE_MATCH_2}")
            set(REPO_VERSION_PATCH "${CMAKE_MATCH_3}")
        endif()
    endif()
endif()

# Set New Version String for use in code and packaging.
set(REPO_VERSION "${REPO_VERSION_MAJOR}.${REPO_VERSION_MINOR}.${REPO_VERSION_PATCH}")

# Include all public headers.
file(GLOB_RECURSE REPO_INC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hxx"
)

# Include all source files and private headers (if any).
file(GLOB_RECURSE REPO_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

# Repository configuration options.
option(GEODESY_GPU_USE_VULKAN_SDK_STATIC_LIB "Link Vulkan SDK statically" OFF)
set(GEODESY_GPU_USE_VULKAN_SDK_VERSION "1.4.321.0" CACHE STRING "Vulkan SDK version to use")
set(REPO_DEPENDENCY_DIR "${CMAKE_SOURCE_DIR}/dep" CACHE PATH "Select Path to store fetched dependency source code. Default is ${CMAKE_SOURCE_DIR}/dep/")

# Declare main project
project(${REPO_NAME}-development
    VERSION ${REPO_VERSION}
    LANGUAGES CXX
)

# Set Project Global Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Utility function for fetching dependencies in one line.
function(fetch_dependency ADEP_NAME AGIT_REPO AGIT_TAG)
    FetchContent_Declare(
        ${ADEP_NAME}
        GIT_REPOSITORY "${AGIT_REPO}"
        GIT_TAG        "${AGIT_TAG}"
        SOURCE_DIR     "${REPO_DEPENDENCY_DIR}/${ADEP_NAME}"
    )

    # Apply cache variables with intelligent type detection and FORCE override
    # CRITICAL: Set these BEFORE FetchContent_MakeAvailable
    foreach(arg IN LISTS ARGN)
        if(arg MATCHES "^([^=]+)=(.*)$")
            set(key "${CMAKE_MATCH_1}")
            set(val "${CMAKE_MATCH_2}")
        else()
            message(FATAL_ERROR
                "Invalid argument passed to fetch_dependency(${ADEP_NAME}): '${arg}'. Expected KEY=VALUE."
            )
        endif()

        if(val MATCHES "^(ON|OFF|TRUE|FALSE)$")
            set(${key} ${val} CACHE BOOL "Forced by fetch_dependency for ${ADEP_NAME}" FORCE)
        else()
            set(${key} "${val}" CACHE STRING "Forced by fetch_dependency for ${ADEP_NAME}" FORCE)
        endif()
    endforeach()

    FetchContent_MakeAvailable(${ADEP_NAME})
endfunction()

# Fetch Dependencies
fetch_dependency(vulkan-headers https://github.com/KhronosGroup/Vulkan-Headers.git "vulkan-sdk-${GEODESY_GPU_USE_VULKAN_SDK_VERSION}")
fetch_dependency(glslang https://github.com/KhronosGroup/glslang.git "vulkan-sdk-${GEODESY_GPU_USE_VULKAN_SDK_VERSION}" ENABLE_PCH=OFF GLSLANG_ENABLE_INSTALL=OFF GLSLANG_TESTS=OFF ENABLE_OPT=OFF)

# Check if user wishes to link Vulkan statically.
if(GEODESY_GPU_USE_VULKAN_SDK_STATIC_LIB)
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        message(STATUS "Geodesy-GPU: Vulkan SDK found.")
        string(FIND "${Vulkan_INCLUDE_DIRS}" "/Include" REMOVE_INDEX REVERSE)
        if(REMOVE_INDEX GREATER -1)
            string(SUBSTRING "${Vulkan_INCLUDE_DIRS}" 0 ${REMOVE_INDEX} VULKAN_SDK_ROOT_DIR)
        else()
            set(VULKAN_SDK_ROOT_DIR "${Vulkan_INCLUDE_DIRS}")
        endif()
        message(STATUS "Geodesy-GPU: Vulkan SDK Root Path: ${VULKAN_SDK_ROOT_DIR}")
    endif()
endif()

add_library(${REPO_NAME} ${REPO_INC} ${REPO_SRC})

target_compile_definitions(${REPO_NAME} PUBLIC
    GEODESY_GPU_NAME="${REPO_NAME}"
    GEODESY_GPU_BRANCH="${REPO_BRANCH}"
    GEODESY_GPU_VERSION="${REPO_VERSION}"
    GEODESY_GPU_VERSION_MAJOR=${REPO_VERSION_MAJOR}
    GEODESY_GPU_VERSION_MINOR=${REPO_VERSION_MINOR}
    GEODESY_GPU_VERSION_PATCH=${REPO_VERSION_PATCH}
)

target_include_directories(${REPO_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${REPO_DEPENDENCY_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
if(GEODESY_GPU_USE_VULKAN_SDK_STATIC_LIB AND Vulkan_FOUND)
    target_include_directories(${REPO_NAME} PUBLIC "${Vulkan_INCLUDE_DIRS}")
else()
    target_include_directories(${REPO_NAME} PUBLIC "${REPO_DEPENDENCY_DIR}/vulkan-headers/include")
endif()

target_link_libraries(${REPO_NAME}
    PUBLIC
        glslang
        SPIRV
)
if(GEODESY_GPU_USE_VULKAN_SDK_STATIC_LIB AND Vulkan_FOUND)
    target_link_libraries(${REPO_NAME} PUBLIC "${Vulkan_LIBRARY}")
endif()

# ==============================================================================
# Unit Tests
# ==============================================================================

# Gather unit test sources
file(GLOB_RECURSE REPO_TST CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tst/*.hxx"
)

# Create unit test executable
add_executable(${REPO_UNIT_TEST_NAME} ${REPO_TST})

# Configure unit test include directories
target_include_directories(${REPO_UNIT_TEST_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/tst"
)

# Link unit test with main library
target_link_libraries(${REPO_UNIT_TEST_NAME}
    PRIVATE
        ${REPO_NAME}
)
