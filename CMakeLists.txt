cmake_minimum_required(VERSION 3.20)

# --------------------------------------------------------------------
# vcpkg Auto-Bootstrap and Toolchain Setup (must come BEFORE project())
# --------------------------------------------------------------------
include(cmake/BootstrapVcpkg.cmake)

# Use vcpkg's toolchain file if available
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_TOOLCHAIN_FILE}" CACHE FILEPATH "vcpkg toolchain file")
set(VCPKG_FEATURE_FLAGS "manifests")

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
# Dependencies (via vcpkg manifest mode)
# --------------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(taskflow CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

# --------------------------------------------------------------------
# Submodules (source-based)
# --------------------------------------------------------------------
add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/implot)
add_subdirectory(thirdparty/nvrhi)

# --------------------------------------------------------------------
# Framework Source
# --------------------------------------------------------------------
include(framework_source.cmake)

# --------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------
message(STATUS "")
message(STATUS "✅ CroissantEngine configured successfully.")
message(STATUS "   Repo root: ${CROISSANT_ROOT_DIR}")
message(STATUS "   Bin path:  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "   Output path: ${OUTPUT_DIR}")
message(STATUS "   Media path:  ${MEDIA_DIR}")