// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RL3D_SRC_RL3D_SHADERS_H_
#define RL3D_SRC_RL3D_SHADERS_H_

#include <raylib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum EmbeddedShader {
    SHADER_GBUF,
    SHADER_DEPTH_DISPLAY,
    SHADER_ADD,
    SHADER_BLUR_HOR,
    SHADER_BLUR_VERT,
    SHADER_SKYBOX,
    SHADER_FLIP_Y,
    SHADER_PHONG_POINT,
} EmbeddedShader;

void LoadShaders();
void UnloadShaders();
Shader GetShader(EmbeddedShader shade);

#ifdef __cplusplus
}
#endif

#endif //RL3D_SRC_RL3D_SHADERS_H_
