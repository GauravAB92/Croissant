
set(DXC_VERSION      "v1.8.2505")
set(DXC_PLATFORM     "windows")       # or "linux" / "mac" as appropriate
set(DXC_ARCH         "x64")           # or "x86"
set(DXC_ZIP_NAME     "dxc_${DXC_VERSION}_${DXC_PLATFORM}_${DXC_ARCH}.zip")
set(DXC_URL          "https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DXC_VERSION}/${DXC_ZIP_NAME}")

set(DXC_THIRDPARTY_DIR     "${CMAKE_SOURCE_DIR}/thirdparty/dxc")
set(DXC_ZIP_PATH           "${DXC_THIRDPARTY_DIR}/${DXC_ZIP_NAME}")
set(DXC_EXTRACT_DIR        "${DXC_THIRDPARTY_DIR}/package")

file(MAKE_DIRECTORY "${DXC_THIRDPARTY_DIR}")
if (NOT EXISTS "${DXC_ZIP_PATH}")
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

if (NOT EXISTS "${DXC_EXTRACT_DIR}")
    message(STATUS "Extracting DXC package to ${DXC_EXTRACT_DIR}…")
    file(MAKE_DIRECTORY "${DXC_EXTRACT_DIR}")
    file(ARCHIVE_EXTRACT
        INPUT      "${DXC_ZIP_PATH}"
        DESTINATION "${DXC_EXTRACT_DIR}"
    )
endif()

if(TARGET DirectX-Headers)
  set_target_properties(DirectX-Headers PROPERTIES FOLDER "Third-Party Libraries")
endif()
if(TARGET DirectX-Guids)
  set_target_properties(DirectX-Guids PROPERTIES FOLDER "Third-Party Libraries")
endif()

set(DXC_BIN_DIR "${DXC_EXTRACT_DIR}/bin/${DXC_ARCH}")
set(DXC_LIB_DIR "${DXC_EXTRACT_DIR}/lib/${DXC_ARCH}")
set(DXC_INCLUDE_DIR "${DXC_EXTRACT_DIR}/inc")

# Imported target for the DXC runtime (dxcompiler.dll with import lib dxcompiler.lib)
add_library(Microsoft::DXCompiler SHARED IMPORTED GLOBAL)
set_target_properties(Microsoft::DXCompiler PROPERTIES
    IMPORTED_IMPLIB         "${DXC_LIB_DIR}/dxcompiler.lib"      # link-time
    IMPORTED_LOCATION       "${DXC_BIN_DIR}/dxcompiler.dll"      # runtime
    INTERFACE_INCLUDE_DIRECTORIES "${DXC_INCLUDE_DIR}"
)

# DXIL validator/runtime (needed by DXC at runtime)
add_library(Microsoft::DXIL SHARED IMPORTED GLOBAL)
set_target_properties(Microsoft::DXIL PROPERTIES
    IMPORTED_LOCATION       "${DXC_BIN_DIR}/dxil.dll"
)


# ---------- Sources ----------

set(FRAMEWORK_SOURCE_DIR "${CMAKE_SOURCE_DIR}/source")
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
set(DXC_THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc")    

file(GLOB_RECURSE FRAMEWORK_SRC CONFIGURE_DEPENDS
    "${FRAMEWORK_SOURCE_DIR}/*.h"    "${FRAMEWORK_SOURCE_DIR}/*.hpp" "${FRAMEWORK_SOURCE_DIR}/*.inl"
    "${FRAMEWORK_SOURCE_DIR}/*.c"    "${FRAMEWORK_SOURCE_DIR}/*.cpp"
)

add_library(framework_source STATIC EXCLUDE_FROM_ALL ${FRAMEWORK_SRC})

target_compile_definitions(framework_source PUBLIC
  NOMINMAX WIN32_LEAN_AND_MEAN
  $<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${NVRHI_WITH_DX12}>>:ENABLE_D3D12=1>
)
# Mirrors the folder tree under 'framework/' in your IDE
source_group(TREE "${FRAMEWORK_SOURCE_DIR}" FILES ${FRAMEWORK_SRC})

target_include_directories(framework_source PUBLIC
    source
    thirdparty
    nvrhi/include
    thirdparty/assimp/include
    thirdparty/glfw/include
    thirdparty/glm/glm
    thirdparty/imgui
    thirdparty/implot   
    "${DXC_INCLUDE_DIR}"
)

# target_link_libraries(framework_source PUBLIC
#     nvrhi_d3d12 dxgi d3d11 glfw assimp imgui implot nvrhi
#     PRIVATE $<$<PLATFORM_ID:Windows>:${DXC_LIB_DIR}/dxcompiler.lib>
# )

target_link_libraries(framework_source PUBLIC
  nvrhi nvrhi_d3d12 dxgi d3d12 imgui implot glfw assimp
  Microsoft::DXCompiler
)

set_target_properties(framework_source PROPERTIES FOLDER FrameworkSource)
target_compile_definitions(framework_source PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)


