cmake_minimum_required(VERSION 3.21)
project(rl3d)

set(CMAKE_C_STANDARD 99)

add_subdirectory(raylib)

add_library(rl3d
        src/rl3d.c
        src/rl3d.h
        src/rl3d_shaders.c
        src/rl3d_shaders.h
        src/rl3d_effects.c
        src/rl3d_effects.h)
target_link_libraries(rl3d raylib)
target_include_directories(rl3d PUBLIC src)

add_executable(rl3d_gbuffer_sphere tests/gbuffer_sphere.cpp)
target_link_libraries(rl3d_gbuffer_sphere rl3d)

add_executable(rl3d_emission_sphere tests/emission_sphere.cpp)
target_link_libraries(rl3d_emission_sphere rl3d)

add_executable(rl3d_skybox tests/skybox.cpp)
target_link_libraries(rl3d_skybox rl3d)
