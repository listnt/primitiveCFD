cmake_minimum_required(VERSION 3.22.1)
project(CFD1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build/)


add_executable(CFD1 main.cpp
        src/base/base.h
        src/base/base.cpp
        src/base/instance.h
        src/base/instance.cpp
        src/base/grid.h
        src/base/grid.cpp
        src/app/app.cpp
        src/app/app.h
        src/base/helper.cpp
        src/base/helper.h
        src/app/CFD.h
        src/app/CFD.cpp
)

add_subdirectory(third_party/imgui)
add_subdirectory(third_party/sdl2webgpu)
target_link_libraries(CFD1 PUBLIC imgui webgpu sdl2webgpu)
#target_link_directories(CFD1 PUBLIC third_party/libs)
target_include_directories(CFD1 PUBLIC third_party)

set(MY_LINK_FLAGS " --preload-file ${PROJECT_SOURCE_DIR}/shaders@/shaders -sUSE_WEBGPU=1 -sUSE_SDL_IMAGE=2 -sUSE_SDL=2 -sASYNCIFY")

IF (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_BUILD_TYPE Debug)
    set(ENV{EMCC_DEBUG} 1)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(MY_LINK_FLAGS "${MY_LINK_FLAGS} --memoryprofiler ")
    add_compile_options(-g)
ENDIF ()

set_target_properties(CFD1 PROPERTIES LINK_FLAGS ${MY_LINK_FLAGS})