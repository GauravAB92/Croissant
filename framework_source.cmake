# -------------------------------------------------------------------------
# Croissant Framework Source Build
# -------------------------------------------------------------------------

# Always anchor paths to this directory (the Croissant root)
set(CROISSANT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# -------------------------------------------------------------------------
# DXC Setup
# -------------------------------------------------------------------------
set(DXC_VERSION      "v1.8.2505")
set(DXC_ZIP_NAME     "dxc_2025_05_24.zip")
set(DXC_URL          "https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DXC_VERSION}/${DXC_ZIP_NAME}")


set(DXC_THIRDPARTY_DIR "${CROISSANT_ROOT_DIR}/thirdparty/dxc")
set(DXC_ZIP_PATH       "${DXC_THIRDPARTY_DIR}/${DXC_ZIP_NAME}")
set(DXC_EXTRACT_DIR    "${DXC_THIRDPARTY_DIR}/package")

file(MAKE_DIRECTORY "${DXC_THIRDPARTY_DIR}")

# Download DXC if missing
if(NOT EXISTS "${DXC_ZIP_PATH}")
    message(STATUS "Downloading DXC ${DXC_VERSION} into ${DXC_ZIP_PATH}…")
    file(DOWNLOAD
        "${DXC_URL}"
        "${DXC_ZIP_PATH}"
        SHOW_PROGRESS
        STATUS _download_status
    )
    list(GET _download_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        message(FATAL_ERROR "DXC download failed: ${_download_status}")
    endif()
endif()

# Extract package if missing
if(NOT EXISTS "${DXC_EXTRACT_DIR}/inc/dxcapi.h")
    message(STATUS "Extracting DXC package to ${DXC_EXTRACT_DIR}…")
    file(MAKE_DIRECTORY "${DXC_EXTRACT_DIR}")
    file(ARCHIVE_EXTRACT
        INPUT "${DXC_ZIP_PATH}"
        DESTINATION "${DXC_EXTRACT_DIR}"
    )
endif()

# -------------------------------------------------------------------------
# DXC Imported Targets
# -------------------------------------------------------------------------
set(DXC_BIN_DIR      "${CROISSANT_ROOT_DIR}/thirdparty/dxc/package/bin/x64")
set(DXC_LIB_DIR      "${CROISSANT_ROOT_DIR}/thirdparty/dxc/package/lib/x64")
set(DXC_INCLUDE_DIR  "${CROISSANT_ROOT_DIR}/thirdparty/dxc/package/inc")

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
# Include Directories (PUBLIC so demos can use them)
# -------------------------------------------------------------------------
target_include_directories(framework_source PUBLIC
    "${CROISSANT_ROOT_DIR}/source"
    "${CROISSANT_ROOT_DIR}/thirdparty"
    "${CROISSANT_ROOT_DIR}/nvrhi/include"

    # 🧩 Include NVRHI's bundled headers FIRST to override old Windows SDK headers
    "${CROISSANT_ROOT_DIR}/nvrhi/thirdparty/DirectX-Headers/include"
    "${CROISSANT_ROOT_DIR}/nvrhi/thirdparty/Vulkan-Headers/include"

    "${CROISSANT_ROOT_DIR}/thirdparty/assimp/include"
    "${CROISSANT_ROOT_DIR}/thirdparty/glfw/include"
    "${CROISSANT_ROOT_DIR}/thirdparty/glm/glm"
    "${CROISSANT_ROOT_DIR}/thirdparty/imgui"
    "${CROISSANT_ROOT_DIR}/thirdparty/implot"
    "${DXC_INCLUDE_DIR}"
)

# -------------------------------------------------------------------------
# Link Libraries
# -------------------------------------------------------------------------
target_link_libraries(framework_source PUBLIC
    nvrhi nvrhi_d3d12
    dxgi d3d12
    imgui implot glfw assimp
    Microsoft::DXCompiler
)

set_target_properties(framework_source PROPERTIES FOLDER "FrameworkSource")

message(STATUS "✅ Croissant framework configured at: ${CROISSANT_ROOT_DIR}")
