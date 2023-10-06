// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RL3D_SRC_RL3D_H_
#define RL3D_SRC_RL3D_H_

#include <raylib.h>

#ifdef __cplusplus
extern "C" {
#endif

// RenderTexture, fbo for texture rendering
typedef struct GBuffers {
    unsigned int id;        // OpenGL framebuffer object id
    Texture albedo;         // Albedo buffer attachment texture
    Texture metallic;       // Metallic buffer attachment texture
    Texture normal;         // Normal buffer attachment texture
    Texture roughness;      // Roughness buffer attachment texture
    Texture emission;       // Emission buffer attachment texture
    Texture ao;             // Ambient occlusion buffer attachment texture
    Texture depth;          // Depth buffer attachment texture
} GBuffers;

typedef struct GBufferPresenter {
    RenderTexture target;
    RenderTexture back[2];
    GBuffers source;
} GBufferPresenter;

void Init3D();
void Uninit3D();

GBuffers LoadGBuffers(int width, int height);
GBufferPresenter LoadPresenter(GBuffers buffers);

Shader GetGBufferShader();

void BeginGBufferMode(GBuffers buffers);
void EndGBufferMode();

void ClearPresenter(GBufferPresenter presenter);
// Renders target to screen
void Present(GBufferPresenter presenter);
// Adds target += back[0]
void AddBackBuffer(GBufferPresenter presenter);

#ifndef NO_CONVENIENCE
void DrawDepth(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
#endif

#ifdef __cplusplus
}
#endif

#endif //RL3D_SRC_RL3D_H_
