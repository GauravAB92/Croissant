# -------------------------------------------------------------------------
# Croissant Framework Source Build
# -------------------------------------------------------------------------

set(CROISSANT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# ---------- DXC Setup ----------
set(DXC_VERSION      "v1.8.2505")
set(DXC_PLATFORM     "windows")
set(DXC_ARCH         "x64")
set(DXC_ZIP_NAME     "dxc_1.8.2505_windows_x64.zip") # ✅ Correct asset name
set(DXC_URL          "https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DXC_VERSION}/${DXC_ZIP_NAME}")

set(DXC_THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc")
set(DXC_ZIP_PATH       "${DXC_THIRDPARTY_DIR}/${DXC_ZIP_NAME}")
set(DXC_EXTRACT_DIR    "${DXC_THIRDPARTY_DIR}/package")

file(MAKE_DIRECTORY "${DXC_THIRDPARTY_DIR}")

# Download only if missing
if(NOT EXISTS "${DXC_ZIP_PATH}")
    message(STATUS "Downloading DXC ${DXC_VERSION} from ${DXC_URL}")
    file(DOWNLOAD "${DXC_URL}" "${DXC_ZIP_PATH}" SHOW_PROGRESS STATUS _download_status)
    list(GET _download_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        message(WARNING "DXC download failed: ${_download_status}")
        message(WARNING "Please manually place ${DXC_ZIP_NAME} in ${DXC_THIRDPARTY_DIR}")
    endif()
endif()

# Extract if not already extracted
if(EXISTS "${DXC_ZIP_PATH}" AND NOT EXISTS "${DXC_EXTRACT_DIR}/inc/dxcapi.h")
    message(STATUS "Extracting DXC package to ${DXC_EXTRACT_DIR}…")
    file(MAKE_DIRECTORY "${DXC_EXTRACT_DIR}")
    file(ARCHIVE_EXTRACT INPUT "${DXC_ZIP_PATH}" DESTINATION "${DXC_EXTRACT_DIR}")
endif()

if(NOT EXISTS "${DXC_EXTRACT_DIR}/inc/dxcapi.h")
    message(FATAL_ERROR "DXC extraction failed — please manually download ${DXC_ZIP_NAME} from ${DXC_URL}")
endif()


# ---------- Framework Sources ----------

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
  $<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${NVRHI_WITH_DX12}>>:ENABLE_D3D12=1>
  _CRT_SECURE_NO_WARNINGS
)

# Mirror folder structure in IDE
source_group(TREE "${FRAMEWORK_SOURCE_DIR}" FILES ${FRAMEWORK_SRC})

# ---------- Include Directories ----------

target_include_directories(framework_source PUBLIC
    "${FRAMEWORK_SOURCE_DIR}"
    "${THIRDPARTY_DIR}"
    "${NVRHI_DIR}/include"
    "${THIRDPARTY_DIR}/assimp/include"
    "${THIRDPARTY_DIR}/glfw/include"
    "${THIRDPARTY_DIR}/glm"
    "${THIRDPARTY_DIR}/imgui"
    "${THIRDPARTY_DIR}/implot"
    "${DXC_INCLUDE_DIR}"
)

# ---------- Link Libraries ----------

target_link_libraries(framework_source PUBLIC
    nvrhi nvrhi_d3d12
    dxgi d3d12
    imgui implot glfw assimp
    Microsoft::DXCompiler
)

set_target_properties(framework_source PROPERTIES FOLDER "FrameworkSource")
