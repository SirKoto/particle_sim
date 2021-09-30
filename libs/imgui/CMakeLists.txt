project(ImGui)

add_library(${PROJECT_NAME}
    imgui.cpp
    imgui_tables.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
    imgui_demo.cpp
    imgui_widgets.cpp
    misc/cpp/imgui_stdlib.cpp
    imgui_draw.cpp
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/
)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

target_link_libraries(${PROJECT_NAME} glad)
target_link_libraries(${PROJECT_NAME} glfw)