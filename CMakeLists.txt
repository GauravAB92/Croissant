cmake_minimum_required(VERSION 3.20)

# --------------------------------------------------------------------
# Detect standalone vs submodule mode
# --------------------------------------------------------------------
set(_is_top_level OFF)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_is_top_level ON)
endif()

# --------------------------------------------------------------------
# vcpkg Setup (local submodule in thirdparty/)
# --------------------------------------------------------------------
set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vcpkg")
set(VCPKG_EXE "${VCPKG_ROOT}/vcpkg.exe")
set(VCPKG_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT EXISTS "${VCPKG_ROOT}")
    message(FATAL_ERROR
        "❌ vcpkg submodule missing.\n"
        "Please run:\n"
        "  git submodule update --init --recursive\n"
    )
endif()

# Bootstrap vcpkg if needed
if(NOT EXISTS "${VCPKG_EXE}")
    message(STATUS "⚙️ Bootstrapping vcpkg...")
    execute_process(
        COMMAND cmd /c bootstrap-vcpkg.bat
        WORKING_DIRECTORY "${VCPKG_ROOT}"
        RESULT_VARIABLE _bootstrap_result
    )
    if(NOT _bootstrap_result EQUAL 0)
        message(FATAL_ERROR "❌ Failed to bootstrap vcpkg (exit ${_bootstrap_result})")
    endif()
endif()

# Ensure dependencies are installed from vcpkg.json
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
    message(STATUS "📦 Ensuring Croissant dependencies are installed via vcpkg manifest...")
    execute_process(
        COMMAND "${VCPKG_EXE}" install --triplet x64-windows --clean-after-build
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _install_result
    )
    if(NOT _install_result EQUAL 0)
        message(FATAL_ERROR "❌ vcpkg install failed (exit ${_install_result})")
    endif()
endif()

# Activate vcpkg toolchain and manifest mode
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_TOOLCHAIN_FILE}" CACHE FILEPATH "vcpkg toolchain" FORCE)
set(VCPKG_FEATURE_FLAGS "manifests,registries" CACHE STRING "")
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ENV{VCPKG_MANIFEST_DIR} "${CMAKE_CURRENT_SOURCE_DIR}")

# --------------------------------------------------------------------
# Project Definition
# --------------------------------------------------------------------
project(CroissantEngine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

# --------------------------------------------------------------------
# vcpkg Include Path Fix
# --------------------------------------------------------------------
set(_vcpkg_installed_dir "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")

if(EXISTS "${_vcpkg_installed_dir}/share")
    list(APPEND CMAKE_PREFIX_PATH "${_vcpkg_installed_dir}/share")
    message(STATUS "🔧 Added vcpkg prefix path: ${_vcpkg_installed_dir}/share")
else()
    message(WARNING "⚠️ Expected vcpkg installed path not found: ${_vcpkg_installed_dir}/share")
endif()

# --------------------------------------------------------------------
# Dependencies (via vcpkg manifest)
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
message(STATUS "   vcpkg root: ${VCPKG_ROOT}")
message(STATUS "   Bin path:   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "   Output path:${OUTPUT_DIR}")
