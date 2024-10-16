// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RL3D_SRC_RL3D_EFFECTS_H_
#define RL3D_SRC_RL3D_EFFECTS_H_

#include <raylib.h>

#include "rl3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Skybox {
    Model box;
} Skybox;

typedef enum ToneMapper {
    TONE_MAP_REINHARD
} ToneMapper;

void RunLightShader(GBufferPresenter presenter, Camera camera, Shader shader);
void RunPostProcessShader(GBufferPresenter presenter, Shader shader);

void BeginLightingPass(GBufferPresenter presenter);
void EndLightingPass();

Skybox LoadSkybox(const char *filename);
Skybox LoadSkyboxImage(Image image);
void DrawSkybox(Skybox skybox, Color tint);

void ShadeFlat(GBufferPresenter presenter);

void ApplyBloom(GBufferPresenter presenter, unsigned int iterations, float radius);
void ApplyGammaCorrection(GBufferPresenter presenter, float gamma);
void ApplyToneMapping(GBufferPresenter presenter, ToneMapper mapper);

void LightPoint(GBufferPresenter presenter, Camera camera, Vector3 position, float intensity, float radius, Color tint);
void LightSun(GBufferPresenter presenter, Camera camera, Vector3 direction, float intensity, Color tint);

#ifdef __cplusplus
}
#endif

#endif //RL3D_SRC_RL3D_EFFECTS_H_
