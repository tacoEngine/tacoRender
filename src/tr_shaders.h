// tacoRender (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TR_SHADERS_H
#define TR_SHADERS_H

#include <raylib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum EmbeddedShader {
    SHADER_GBUF,
    SHADER_DEPTH_DISPLAY,
    SHADER_ADD,
    SHADER_BLUR_GAUSS,
    SHADER_BLUR_BOX,
    SHADER_SKYBOX,
    SHADER_FLIP_Y,
    SHADER_POINT,
    SHADER_SUN,
    SHADER_IBL,
    SHADER_GAMMA,
    SHADER_TONE_MAP_REINHARD,
    SHADER_IRRADIANCE,
    SHADER_PREFILTER,
    SHADER_SSAO,
    SHADER_TEX_TO_DEPTH,
} EmbeddedShader;

void LoadShaders();
void UnloadShaders();
Shader GetShader(EmbeddedShader shade);

#ifdef __cplusplus
}
#endif

#endif //TR_SHADERS_H
