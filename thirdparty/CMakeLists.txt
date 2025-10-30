# -------------------------------------------------------------------------
# Croissant Third-Party Dependencies
# -------------------------------------------------------------------------

if(THIRDPARTY_CMAKELISTS_INCLUDED)
    return()
endif()
set(THIRDPARTY_CMAKELISTS_INCLUDED ON)

set(third_party_folder "Third-Party Libraries" CACHE STRING "Visual Studio folder for all 3rd-party libs")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static libs" FORCE)

# -------------------------------------------------------------------------
# ImGui / ImPlot
# -------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui.cmake")
    include(imgui.cmake)
    set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder})
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/implot.cmake")
    include(implot.cmake)
    set_target_properties(implot PROPERTIES FOLDER ${third_party_folder})
endif()

# -------------------------------------------------------------------------
# Assimp
# -------------------------------------------------------------------------
include(CMakeDependentOption)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES         OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS           OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER   ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_EXPORTER   OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assimp/CMakeLists.txt")
    add_subdirectory(assimp)
    set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder})
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ${third_party_folder})
    set_target_properties(zlibstatic PROPERTIES FOLDER ${third_party_folder})
    set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder})
else()
    message(WARNING "Assimp not found — skipping.")
endif()

# -------------------------------------------------------------------------
# GLFW
# -------------------------------------------------------------------------
option(GLFW_BUILD_EXAMPLES "GLFW examples" OFF)
option(GLFW_BUILD_TESTS "GLFW tests" OFF)
option(GLFW_BUILD_DOCS "GLFW docs" OFF)
option(GLFW_INSTALL "GLFW install" OFF)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw/CMakeLists.txt")
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
    set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder})
else()
    message(WARNING "GLFW not found — skipping.")
endif()

# -------------------------------------------------------------------------
# GLM
# -------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glm/CMakeLists.txt")
    add_subdirectory(glm)
    set_target_properties(glm PROPERTIES FOLDER ${third_party_folder})
else()
    message(WARNING "GLM not found — skipping.")
endif()
