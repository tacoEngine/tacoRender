cmake_minimum_required(VERSION 3.21)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(tacoRender)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)

set(CUSTOMIZE_BUILD ON)
set(SUPPORT_FILEFORMAT_HDR ON)
set(SUPPORT_FILEFORMAT_JPG ON)
add_subdirectory(raylib)

add_library(tacoRender
        src/tacoRender.c
        src/tacoRender.h
        src/tr_shaders.c
        src/tr_shaders.h
        src/tr_effects.c
        src/tr_effects.h
        src/tr_generators.c
        src/tr_generators.h
        src/tr_timing.c
        src/tr_timing.h
        src/tr_math.c
        src/tr_math.h
)
target_link_libraries(tacoRender raylib)
target_include_directories(tacoRender PUBLIC src)

add_executable(tr_gbuffer_sphere examples/gbuffer_sphere.cpp)
target_link_libraries(tr_gbuffer_sphere tacoRender)

add_executable(tr_skybox examples/skybox.cpp)
target_link_libraries(tr_skybox tacoRender)

add_executable(tr_lit_sphere examples/lit_sphere.cpp)
target_link_libraries(tr_lit_sphere tacoRender)

add_executable(tr_hdr_plane examples/hdr_plane.cpp)
target_link_libraries(tr_hdr_plane tacoRender)

add_executable(tr_pbr_sphere examples/pbr_sphere.cpp)
target_link_libraries(tr_pbr_sphere tacoRender)

add_executable(tr_ibl_spheres examples/ibl_spheres.cpp)
target_link_libraries(tr_ibl_spheres tacoRender)

add_executable(tr_ibl_model examples/ibl_model.cpp)
target_link_libraries(tr_ibl_model tacoRender)

add_executable(tr_cubemap_gen examples/cubemap_generation.cpp)
target_link_libraries(tr_cubemap_gen tacoRender)

add_executable(tr_shadowmap examples/shadow_map.cpp)
target_link_libraries(tr_shadowmap tacoRender)

add_executable(tr_monkey_ao examples/monkey_ao.cpp)
target_link_libraries(tr_monkey_ao tacoRender)

add_executable(tr_sss_point examples/screen_space_shadows.cpp)
target_link_libraries(tr_sss_point tacoRender)
