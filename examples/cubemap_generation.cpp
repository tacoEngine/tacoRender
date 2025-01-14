// tacoRender (c) Nikolas Wipper 2024

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <tacoRender.h>
#include <tr_effects.h>

#include <rlgl.h>
#include <external/glad.h>

#include "tr_generators.h"

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "tacoRender emissions sphere");
    Init3D();

    Camera3D camera = (Camera3D){
        .position = (Vector3){0, 2, -5}, .target = (Vector3){0, 0, 0}, .up = (Vector3){
            0, 1, 0
        },
        .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
    };

    Model spheres[4];

    for (int i = 0; i < 4; i++) {
        spheres[i] = LoadModelFromMesh(GenMeshSphere(1, 50, 50));

        MaterialMap *maps = spheres[i].materials[0].maps;

        spheres[i].materials[0].shader = GetGBufferShader();
        maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(GenImageColor(1, 1, BROWN));
        maps[MATERIAL_MAP_NORMAL].texture = LoadTextureFromImage(GenImageColor(1, 1, {128, 128, 255}));

        unsigned char met_channel = i % 2 * 128 + 64;
        unsigned char rough_channel = i / 2 * 128 + 64;

        Color metallic = Color{met_channel, met_channel, met_channel, 255};
        Color rough = Color{rough_channel, rough_channel, rough_channel, 255};

        maps[MATERIAL_MAP_METALNESS].texture = LoadTextureFromImage(GenImageColor(1, 1, metallic));
        maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTextureFromImage(GenImageColor(1, 1, rough));
        maps[MATERIAL_MAP_EMISSION].texture = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
        maps[MATERIAL_MAP_OCCLUSION].texture = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
    }

    TextureCubemap env = LoadTextureCubemap(LoadImage("examples/assets/ibl/skybox.hdr"), CUBEMAP_LAYOUT_AUTO_DETECT);

    TextureCubemap radiance = PrefilterCubemap(env);
    TextureCubemap irradiance = IrradianceCubemap(env);

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            for (int i = 0; i < 4; i++) {
                DrawModel(spheres[i], (Vector3){(float)(i % 2) - 0.5f, (float)(i / 2) - 0.5f, 0}, 0.25f, WHITE);
            }

            EndMode3D();

            EndGBufferMode();
        }

        ClearPresenter(presenter);

        { // Shade scene
            BeginLightingPass(presenter);

            ClearBackground(BLACK);

            LightIBL(presenter, camera, radiance, irradiance);

            EndLightingPass();
        }

        AddBackBuffer(presenter);

        ApplyToneMapping(presenter, TONE_MAP_REINHARD);
        ApplyGammaCorrection(presenter, 2.2);

        // Render GBuffers to screen
        Present(presenter);
    }

    Uninit3D();
    CloseWindow();
}
