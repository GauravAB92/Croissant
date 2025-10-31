# -------------------------------------------------------------------------
# Croissant Framework Source Build
# -------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

# Always anchor paths relative to this file
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
set(CROISSANT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# -------------------------------------------------------------------------
# DXC Setup (Windows-only)
# -------------------------------------------------------------------------
if(WIN32)
    set(DXC_VERSION      "v1.8.2505")
    set(DXC_ZIP_NAME     "dxc_2025_05_24.zip")
    set(DXC_URL          "https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DXC_VERSION}/${DXC_ZIP_NAME}")

    set(DXC_THIRDPARTY_DIR "${CROISSANT_ROOT_DIR}/thirdparty/dxc")
    set(DXC_ZIP_PATH       "${DXC_THIRDPARTY_DIR}/${DXC_ZIP_NAME}")
    set(DXC_EXTRACT_DIR    "${DXC_THIRDPARTY_DIR}/package")

    file(MAKE_DIRECTORY "${DXC_THIRDPARTY_DIR}")

    if(NOT EXISTS "${DXC_ZIP_PATH}")
        message(STATUS "Downloading DXC ${DXC_VERSION} into ${DXC_ZIP_PATH}…")
        set(_download_status "")
        file(DOWNLOAD
            "${DXC_URL}" "${DXC_ZIP_PATH}"
            SHOW_PROGRESS STATUS _download_status
        )
        list(GET _download_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            message(FATAL_ERROR "DXC download failed: ${_download_status}")
        endif()
    endif()

    if(NOT EXISTS "${DXC_EXTRACT_DIR}/inc/dxcapi.h")
        message(STATUS "Extracting DXC package to ${DXC_EXTRACT_DIR}…")
        file(MAKE_DIRECTORY "${DXC_EXTRACT_DIR}")
        file(ARCHIVE_EXTRACT INPUT "${DXC_ZIP_PATH}" DESTINATION "${DXC_EXTRACT_DIR}")
    endif()

    set(DXC_BIN_DIR      "${DXC_EXTRACT_DIR}/bin/x64")
    set(DXC_LIB_DIR      "${DXC_EXTRACT_DIR}/lib/x64")
    set(DXC_INCLUDE_DIR  "${DXC_EXTRACT_DIR}/inc")

    if(EXISTS "${DXC_LIB_DIR}/dxcompiler.lib" AND EXISTS "${DXC_BIN_DIR}/dxcompiler.dll")
        if(NOT TARGET Microsoft::DXCompiler)
            add_library(Microsoft::DXCompiler SHARED IMPORTED GLOBAL)
            set_target_properties(Microsoft::DXCompiler PROPERTIES
                IMPORTED_IMPLIB               "${DXC_LIB_DIR}/dxcompiler.lib"
                IMPORTED_LOCATION             "${DXC_BIN_DIR}/dxcompiler.dll"
                INTERFACE_INCLUDE_DIRECTORIES "${DXC_INCLUDE_DIR}"
            )
        endif()
    else()
        message(WARNING "⚠️ DXC binaries not found: ${DXC_LIB_DIR}/dxcompiler.lib or ${DXC_BIN_DIR}/dxcompiler.dll")
    endif()

    if(EXISTS "${DXC_BIN_DIR}/dxil.dll" AND NOT TARGET Microsoft::DXIL)
        add_library(Microsoft::DXIL SHARED IMPORTED GLOBAL)
        set_target_properties(Microsoft::DXIL PROPERTIES
            IMPORTED_LOCATION "${DXC_BIN_DIR}/dxil.dll"
        )
    endif()
endif()

# -------------------------------------------------------------------------
# Framework Source Files
# -------------------------------------------------------------------------
set(FRAMEWORK_SOURCE_DIR "${CROISSANT_ROOT_DIR}/source")
set(THIRDPARTY_DIR       "${CROISSANT_ROOT_DIR}/thirdparty")
set(NVRHI_DIR            "${CROISSANT_ROOT_DIR}/nvrhi")

file(GLOB_RECURSE FRAMEWORK_SRC CONFIGURE_DEPENDS
    "${FRAMEWORK_SOURCE_DIR}/*.h"
    "${FRAMEWORK_SOURCE_DIR}/*.hpp"
    "${FRAMEWORK_SOURCE_DIR}/*.inl"
    "${FRAMEWORK_SOURCE_DIR}/*.c"
    "${FRAMEWORK_SOURCE_DIR}/*.cpp"
)

add_library(framework_source STATIC EXCLUDE_FROM_ALL ${FRAMEWORK_SRC})

target_compile_definitions(framework_source PUBLIC
    NOMINMAX WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    $<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${NVRHI_WITH_DX12}>>:ENABLE_D3D12=1>
)

# Keep folder structure in IDE
source_group(TREE "${FRAMEWORK_SOURCE_DIR}" FILES ${FRAMEWORK_SRC})

# -------------------------------------------------------------------------
# Include Directories (PUBLIC)
# -------------------------------------------------------------------------
target_include_directories(framework_source PUBLIC
    "${FRAMEWORK_SOURCE_DIR}"
    "${NVRHI_DIR}/include"
    "${NVRHI_DIR}/thirdparty/DirectX-Headers/include"
    "${NVRHI_DIR}/thirdparty/Vulkan-Headers/include"
    "${THIRDPARTY_DIR}/imgui"
    "${THIRDPARTY_DIR}/implot"
    "${THIRDPARTY_DIR}/assimp/include"
    $<$<BOOL:WIN32>:${DXC_INCLUDE_DIR}>
)

# -------------------------------------------------------------------------
# Link Libraries
# -------------------------------------------------------------------------
target_link_libraries(framework_source PUBLIC
    nvrhi
    imgui
    implot
    glm::glm
    glfw
    assimp::assimp
    Taskflow::Taskflow
)

if(WIN32 AND NVRHI_WITH_DX12)
    target_link_libraries(framework_source PUBLIC nvrhi_d3d12 dxgi d3d12)
endif()

if(TARGET Microsoft::DXCompiler)
    target_link_libraries(framework_source PUBLIC Microsoft::DXCompiler)
endif()

set_target_properties(framework_source PROPERTIES FOLDER "FrameworkSource")

message(STATUS "✅ Croissant framework configured at: ${CROISSANT_ROOT_DIR}")
