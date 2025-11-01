cmake_minimum_required(VERSION 3.20)
project(CroissantEngine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --------------------------------------------------------------------
# Detect standalone vs submodule
# --------------------------------------------------------------------
set(_is_top_level OFF)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_is_top_level ON)
endif()

# --------------------------------------------------------------------
# vcpkg Setup (thirdparty submodule)
# --------------------------------------------------------------------
set(_vcpkg_root "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vcpkg")
set(_vcpkg_toolchain "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")

# Ensure vcpkg is present
if(NOT EXISTS "${_vcpkg_toolchain}")
    message(STATUS "📦 Cloning vcpkg submodule into thirdparty/ ...")
    execute_process(
        COMMAND git submodule update --init --recursive thirdparty/vcpkg
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _submodule_result
    )
    if(NOT _submodule_result EQUAL 0)
        message(FATAL_ERROR "❌ Failed to initialize vcpkg submodule (exit ${_submodule_result})")
    endif()
endif()

# Bootstrap vcpkg if not yet built
if(WIN32)
    set(_vcpkg_exe "${_vcpkg_root}/vcpkg.exe")
    if(NOT EXISTS "${_vcpkg_exe}")
        message(STATUS "⚙️ Bootstrapping vcpkg (Windows)...")
        execute_process(
            COMMAND cmd /c bootstrap-vcpkg.bat
            WORKING_DIRECTORY "${_vcpkg_root}"
            RESULT_VARIABLE _boot_result
        )
        if(NOT _boot_result EQUAL 0)
            message(FATAL_ERROR "❌ Failed to bootstrap vcpkg (exit ${_boot_result})")
        endif()
    endif()
else()
    set(_vcpkg_exe "${_vcpkg_root}/vcpkg")
    if(NOT EXISTS "${_vcpkg_exe}")
        message(STATUS "⚙️ Bootstrapping vcpkg (Unix)...")
        execute_process(
            COMMAND bash ./bootstrap-vcpkg.sh
            WORKING_DIRECTORY "${_vcpkg_root}"
            RESULT_VARIABLE _boot_result
        )
        if(NOT _boot_result EQUAL 0)
            message(FATAL_ERROR "❌ Failed to bootstrap vcpkg (exit ${_boot_result})")
        endif()
    endif()
endif()

# Install dependencies (manifest mode)
message(STATUS "📦 Ensuring Croissant dependencies are installed...")
execute_process(
    COMMAND "${_vcpkg_exe}" install --triplet x64-windows --clean-after-build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _install_result
)
if(NOT _install_result EQUAL 0)
    message(FATAL_ERROR "❌ vcpkg install failed (exit ${_install_result})")
endif()

# Use vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_toolchain}" CACHE FILEPATH "vcpkg toolchain file" FORCE)
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(VCPKG_FEATURE_FLAGS "manifests,registries")

# --------------------------------------------------------------------
# Debug Info
# --------------------------------------------------------------------
message(STATUS "======== vcpkg DEBUG INFO ========")
message(STATUS "VCPKG_ROOT            = ${_vcpkg_root}")
message(STATUS "CMAKE_TOOLCHAIN_FILE  = ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_MANIFEST_DIR    = ${VCPKG_MANIFEST_DIR}")
message(STATUS "==============================")

# --------------------------------------------------------------------
# Dependencies (via manifest)
# --------------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(taskflow CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

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

message(STATUS "")
message(STATUS "✅ CroissantEngine configured successfully.")
