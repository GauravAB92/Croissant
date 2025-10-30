set(implot_srcs
    ${CMAKE_CURRENT_SOURCE_DIR}/implot/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/implot/implot_items.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/implot/implot_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/implot/implot.h
)

add_library(implot STATIC ${implot_srcs})
target_include_directories(
    implot PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/implot
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
