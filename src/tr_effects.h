// tacoRender (c) Nikolas Wipper 2023-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TR_EFFECTS_H
#define TR_EFFECTS_H

#include <raylib.h>

#include "tacoRender.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {



#endif

typedef struct Skybox {
    Model box;
} Skybox;

typedef enum ToneMapper {
    TONE_MAP_REINHARD
} ToneMapper;

typedef enum Blur {
    BLUR_BOX,
    BLUR_BOX_DEPTH,
    BLUR_GAUSS,
} Blur;

typedef struct ShadowMap {
    unsigned int fbo;

    unsigned int *ids;
    RenderTexture back[2];
    Matrix *projections;
    float *dists;
    int size;
    int cascades;
} ShadowMap;

#define NULL_SHADOW_MAP ((ShadowMap) {0, NULL, NULL, NULL, 0, 0})

void RunLightShader(GBufferPresenter presenter, Camera camera, Shader shader);
void RunLightShaderEx(GBufferPresenter presenter,
                      Camera camera,
                      Shader shader,
                      int extraTextureCount,
                      unsigned int *extraTextureIDs,
                      int *extraTextureLocs);
void RunLightShaderPro(GBufferPresenter presenter,
                       Camera camera,
                       Shader shader,
                       int cubemapCount,
                       unsigned int *cubemapIDs,
                       int *cubemapLocs,
                       int extraTextureCount,
                       unsigned int *extraTextureIDs,
                       int *extraTextureLocs);
void RunPostProcessShader(GBufferPresenter presenter, Shader shader);
Texture BlurTexture(Blur blur, Texture texture, RenderTexture back[], unsigned int iterations);

void BeginLightingPass(GBufferPresenter presenter);
void EndLightingPass();

ShadowMap LoadShadowMap(int size, int cascades, float cascadeDistance);
void UnloadShadowMap(ShadowMap);
void BeginShadowMap(ShadowMap shadowMap, Camera camera, Vector3 lightDirection, int cascade);
void EndShadowMap();
void FilterShadowMap(ShadowMap shadowMap, unsigned int iterations);

Skybox LoadSkybox(const char *filename);
Skybox LoadSkyboxImage(Image image);
void DrawSkybox(Skybox skybox, Color tint);

void ShadeFlat(GBufferPresenter presenter);
void CopyBackground(GBufferPresenter presenter);

void ApplyGammaCorrection(GBufferPresenter presenter, float gamma);
void ApplyToneMapping(GBufferPresenter presenter, ToneMapper mapper);
void ApplySSAO(GBufferPresenter presenter, Camera camera);

void LightPoint(GBufferPresenter presenter, Camera camera, Vector3 position, float intensity, float radius, Color tint);
void LightSun(GBufferPresenter presenter,
              Camera camera,
              Vector3 direction,
              float intensity,
              Color tint,
              ShadowMap shadowMap);
void LightIBL(GBufferPresenter presenter, Camera camera, TextureCubemap radiance, TextureCubemap irradiance);

#ifdef __cplusplus
}
#endif

#endif //TR_EFFECTS_H
