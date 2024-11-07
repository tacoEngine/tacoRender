cmake_minimum_required(VERSION 3.21)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(rl3d)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)

set(CUSTOMIZE_BUILD ON)
set(SUPPORT_FILEFORMAT_HDR ON)
add_subdirectory(raylib)

add_library(rl3d
        src/rl3d.c
        src/rl3d.h
        src/rl3d_shaders.c
        src/rl3d_shaders.h
        src/rl3d_effects.c
        src/rl3d_effects.h
        src/rl3d_generators.c
        src/rl3d_generators.h
)
target_link_libraries(rl3d raylib)
target_include_directories(rl3d PUBLIC src)

add_executable(rl3d_gbuffer_sphere tests/gbuffer_sphere.cpp)
target_link_libraries(rl3d_gbuffer_sphere rl3d)

add_executable(rl3d_emission_sphere tests/emission_sphere.cpp)
target_link_libraries(rl3d_emission_sphere rl3d)

add_executable(rl3d_skybox tests/skybox.cpp)
target_link_libraries(rl3d_skybox rl3d)

add_executable(rl3d_lit_sphere tests/lit_sphere.cpp)
target_link_libraries(rl3d_lit_sphere rl3d)

add_executable(rl3d_hdr_plane tests/hdr_plane.cpp)
target_link_libraries(rl3d_hdr_plane rl3d)

add_executable(rl3d_pbr_sphere tests/pbr_sphere.cpp)
target_link_libraries(rl3d_pbr_sphere rl3d)

add_executable(rl3d_ibl_spheres tests/ibl_spheres.cpp)
target_link_libraries(rl3d_ibl_spheres rl3d)
