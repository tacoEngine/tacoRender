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

Skybox LoadSkybox(const char *filename);
Skybox LoadSkyboxImage(Image image);
void DrawSkybox(Skybox skybox, Color tint);

void ShadeFlat(GBufferPresenter presenter);

void ApplyBloom(GBufferPresenter presenter, unsigned int iterations, float radius);
void ApplyGammaCorrection(GBufferPresenter presenter, float gamma);

void RunSingleShader(GBufferPresenter presenter, Camera camera, Shader shader);

void LightPoint(GBufferPresenter presenter, Camera camera, Vector3 position, float intensity, Color tint);
void LightSun(GBufferPresenter presenter, Camera camera, Vector3 direction, float intensity, Color tint);

#ifdef __cplusplus
}
#endif

#endif //RL3D_SRC_RL3D_EFFECTS_H_
