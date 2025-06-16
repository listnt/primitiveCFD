set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_sdl2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_opengl3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp"
)
add_library(imgui STATIC ${SOURCES})