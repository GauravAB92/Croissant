cmake_minimum_required(VERSION 3.20)

# --------------------------------------------------------------------
# Detect standalone vs submodule mode
# --------------------------------------------------------------------
set(_is_top_level OFF)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_is_top_level ON)
endif()

# --------------------------------------------------------------------
# vcpkg Setup
# --------------------------------------------------------------------
if(_is_top_level)
    # ✅ Standalone mode: bootstrap and activate vcpkg
    include("${CMAKE_CURRENT_SOURCE_DIR}/BootstrapVcpkg.cmake")

    set(CMAKE_TOOLCHAIN_FILE
        "${VCPKG_TOOLCHAIN_FILE}"
        CACHE FILEPATH "vcpkg toolchain file" FORCE
    )

    # Enable manifest mode
    set(VCPKG_FEATURE_FLAGS "manifests,registries")
    set(ENV{VCPKG_FEATURE_FLAGS} "manifests,registries")
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(ENV{VCPKG_MANIFEST_DIR} "${VCPKG_MANIFEST_DIR}")
else()
    # ✅ Submodule mode: if vcpkg toolchain wasn’t inherited, inject local prefix manually
    if(NOT DEFINED CMAKE_PREFIX_PATH OR CMAKE_PREFIX_PATH STREQUAL "")
        set(_vcpkg_prefix "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/share")
        if(EXISTS "${_vcpkg_prefix}")
            message(STATUS "🔧 Submodule mode: adding vcpkg prefix ${_vcpkg_prefix}")
            list(APPEND CMAKE_PREFIX_PATH "${_vcpkg_prefix}")
        else()
            message(WARNING "⚠️ vcpkg prefix not found at ${_vcpkg_prefix}")
        endif()
    endif()
endif()

# --------------------------------------------------------------------
# Project Definition
# --------------------------------------------------------------------
project(CroissantEngine VERSION 0.1 LANGUAGES CXX)

# --------------------------------------------------------------------
# Global Settings
# --------------------------------------------------------------------
set(CROISSANT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(CROISSANT_PARENT_DIR "${CROISSANT_ROOT_DIR}" DIRECTORY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

# --------------------------------------------------------------------
# Output Directories (outside repo)
# --------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CROISSANT_PARENT_DIR}/bin")
set(OUTPUT_DIR                     "${CROISSANT_PARENT_DIR}/output")
set(MEDIA_DIR                      "${CROISSANT_PARENT_DIR}/media")

foreach(_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" "${OUTPUT_DIR}" "${MEDIA_DIR}")
    if(NOT EXISTS "${_dir}")
        file(MAKE_DIRECTORY "${_dir}")
    endif()
endforeach()

# --------------------------------------------------------------------
# Engine Options
# --------------------------------------------------------------------
option(NVRHI_WITH_DX12 "Enable NVRHI DX12 backend" ON)
option(NVRHI_WITH_DX11 "Enable NVRHI DX11 backend" OFF)
option(NVRHI_WITH_VULKAN "Enable NVRHI Vulkan backend" OFF)
option(NVRHI_WITH_AFTERMATH "Enable NVRHI Aftermath support" OFF)

# --------------------------------------------------------------------
# Debug Info
# --------------------------------------------------------------------
message(STATUS "======== vcpkg DEBUG INFO ========")
message(STATUS "CMAKE_SOURCE_DIR         = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "VCPKG_TOOLCHAIN_FILE     = ${VCPKG_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_MANIFEST_DIR       = ${VCPKG_MANIFEST_DIR}")
message(STATUS "CMAKE_PREFIX_PATH        = ${CMAKE_PREFIX_PATH}")
message(STATUS "==============================")

# --------------------------------------------------------------------
# Dependencies (via vcpkg)
# --------------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(taskflow CONFIG REQUIRED)

# --------------------------------------------------------------------
# Submodules (source-based)
# --------------------------------------------------------------------
add_subdirectory(thirdparty)
add_subdirectory(nvrhi)

# --------------------------------------------------------------------
# Framework Source
# --------------------------------------------------------------------
include(framework_source.cmake)

# --------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------
message(STATUS "")
message(STATUS "✅ CroissantEngine configured successfully.")
message(STATUS "   Repo root:  ${CROISSANT_ROOT_DIR}")
message(STATUS "   Bin path:   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "   Output path:${OUTPUT_DIR}")
message(STATUS "   Media path: ${MEDIA_DIR}")
