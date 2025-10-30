set(third_party_folder "Third-Party Libraries")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


include(imgui.cmake)
set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder})

include(implot.cmake)
set_target_properties(implot PROPERTIES FOLDER ${third_party_folder})

include(CMakeDependentOption)

# disable every importer/exporter by default
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

# disable the tools/examples/tests so you only get the library
set(ASSIMP_BUILD_ASSIMP_TOOLS    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES         OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS           OFF CACHE BOOL "" FORCE)

# now explicitly turn on just the glTF importer
set(ASSIMP_BUILD_GLTF_IMPORTER   ON CACHE BOOL "" FORCE)

# (optionally disable the glTF exporter if you don't need it)
set(ASSIMP_BUILD_GLTF_EXPORTER   OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp)
set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder})
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ${third_party_folder})
set_target_properties(zlibstatic PROPERTIES FOLDER ${third_party_folder})
set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder})


option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)
set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder})


add_subdirectory(glm)
set_target_properties(glm PROPERTIES FOLDER ${third_party_folder})
