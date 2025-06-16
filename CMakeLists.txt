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
target_link_directories(CFD1 PUBLIC third_party/libs)
target_include_directories(CFD1 PUBLIC third_party)

target_compile_options(CFD1 PUBLIC -fno-stack-protector)
