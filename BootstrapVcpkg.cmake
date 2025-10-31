# =============================================================================
# BootstrapVcpkg.cmake
# Ensures vcpkg is available, bootstrapped, and installs manifest dependencies.
# - Uses system vcpkg if found
# - Otherwise clones locally (pinned or latest)
# - Works on Windows, Linux, and macOS
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# 1. Locate or clone vcpkg
# -----------------------------------------------------------------------------
set(_candidate_roots
    "$ENV{VCPKG_ROOT}"
    "$ENV{USERPROFILE}/vcpkg"
    "$ENV{HOME}/vcpkg"
    "${CMAKE_CURRENT_LIST_DIR}/../vcpkg"
)
set(VCPKG_ROOT "")
foreach(_path IN LISTS _candidate_roots)
    if(EXISTS "${_path}/vcpkg${CMAKE_EXECUTABLE_SUFFIX}")
        set(VCPKG_ROOT "${_path}")
        message(STATUS "✅ Found existing vcpkg at: ${VCPKG_ROOT}")
        break()
    endif()
endforeach()

if(NOT VCPKG_ROOT)
    set(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../vcpkg")
    message(STATUS "⬇️ vcpkg not found — cloning into ${VCPKG_ROOT} ...")
    find_package(Git REQUIRED)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"
        RESULT_VARIABLE clone_result
    )
    if(clone_result)
        message(FATAL_ERROR "❌ Failed to clone vcpkg (exit code ${clone_result}).")
    endif()
endif()

# -----------------------------------------------------------------------------
# 2. Bootstrap the vcpkg executable if needed
# -----------------------------------------------------------------------------
if(WIN32)
    set(VCPKG_EXE "${VCPKG_ROOT}/vcpkg.exe")
else()
    set(VCPKG_EXE "${VCPKG_ROOT}/vcpkg")
endif()

if(NOT EXISTS "${VCPKG_EXE}")
    message(STATUS "⚙️ Bootstrapping vcpkg (this may take a minute)...")
    if(WIN32)
        execute_process(
            COMMAND cmd /c bootstrap-vcpkg.bat
            WORKING_DIRECTORY "${VCPKG_ROOT}"
            RESULT_VARIABLE bootstrap_result
        )
    else()
        execute_process(
            COMMAND bash ./bootstrap-vcpkg.sh
            WORKING_DIRECTORY "${VCPKG_ROOT}"
            RESULT_VARIABLE bootstrap_result
        )
    endif()
    if(bootstrap_result)
        message(FATAL_ERROR "❌ vcpkg bootstrap failed (exit code ${bootstrap_result})")
    endif()
    message(STATUS "✅ vcpkg bootstrap complete.")
else()
    message(STATUS "Using existing vcpkg executable at: ${VCPKG_EXE}")
endif()

# -----------------------------------------------------------------------------
# 3. Install manifest dependencies (Croissant/vcpkg.json)
# -----------------------------------------------------------------------------
set(_vcpkg_manifest "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
if(EXISTS "${_vcpkg_manifest}")
    message(STATUS "📦 Installing dependencies from manifest: ${_vcpkg_manifest}")
    execute_process(
        COMMAND "${VCPKG_EXE}" install --triplet x64-windows --clean-after-build
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _vcpkg_install_result
    )
    if(NOT _vcpkg_install_result EQUAL 0)
        message(FATAL_ERROR "❌ vcpkg install failed with code ${_vcpkg_install_result}")
    endif()
else()
    message(WARNING "⚠️ No vcpkg.json found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# -----------------------------------------------------------------------------
# 4. Export toolchain path for parent CMakeLists.txt
# -----------------------------------------------------------------------------
set(VCPKG_TOOLCHAIN_FILE
    "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE FILEPATH "vcpkg toolchain file"
)
message(STATUS "🧰 Using vcpkg toolchain: ${VCPKG_TOOLCHAIN_FILE}")
