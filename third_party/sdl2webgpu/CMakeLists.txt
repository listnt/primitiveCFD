set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sdl2webgpu.c"
)

add_library(sdl2webgpu STATIC ${SOURCES})
target_include_directories(sdl2webgpu PUBLIC .)
target_link_libraries(sdl2webgpu PUBLIC SDL2 webgpu)