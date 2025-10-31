# -------------------------------------------------------------------------
# Bootstrap vcpkg automatically if it's missing
# -------------------------------------------------------------------------
set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
set(VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg${CMAKE_EXECUTABLE_SUFFIX}")

if(NOT EXISTS "${VCPKG_EXECUTABLE}")
    message(STATUS "vcpkg not found — downloading and bootstrapping it...")

    file(DOWNLOAD
        "https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip"
        "${CMAKE_BINARY_DIR}/vcpkg.zip"
        SHOW_PROGRESS
    )

    file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/vcpkg.zip"
                         DESTINATION "${CMAKE_BINARY_DIR}/vcpkg-src")

    # The archive extracts into vcpkg-master/
    file(GLOB _vcpkg_dirs "${CMAKE_BINARY_DIR}/vcpkg-src/*")
    list(GET _vcpkg_dirs 0 VCPKG_SRC_DIR)
    file(RENAME "${VCPKG_SRC_DIR}" "${VCPKG_ROOT}")

    message(STATUS "Bootstrapping vcpkg (this may take a minute)...")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E chdir "${VCPKG_ROOT}" ./bootstrap-vcpkg.bat
        RESULT_VARIABLE bootstrap_result
        OUTPUT_VARIABLE bootstrap_out
        ERROR_VARIABLE  bootstrap_err
    )
    if(bootstrap_result)
        message(FATAL_ERROR "vcpkg bootstrap failed:\n${bootstrap_err}")
    endif()
endif()

set(VCPKG_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
message(STATUS "Using vcpkg at: ${VCPKG_TOOLCHAIN_FILE}")
