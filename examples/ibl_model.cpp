// tacoRender (c) Nikolas Wipper 2023-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cstdio>
#include <tacoRender.h>
#include <tr_effects.h>

#include "raymath.h"
#include "tr_generators.h"

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 1000;

    InitWindow(screenWidth, screenHeight, "tacoRender emissions sphere");
    Init3D();

    Camera3D camera = (Camera3D){
        .position = (Vector3){0, 2, -5}, .target = (Vector3){0, 0, 0}, .up = (Vector3){
            0, 1, 0
        },
        .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
    };

    Model model = LoadModel("examples/assets/model/DamagedHelmet.glb");

    TextureCubemap radiance = LoadTextureCubemap(LoadImage("examples/assets/ibl/radiance.dds"), CUBEMAP_LAYOUT_AUTO_DETECT);
    TextureCubemap irradiance = LoadTextureCubemap(LoadImage("examples/assets/ibl/irradiance.hdr"), CUBEMAP_LAYOUT_AUTO_DETECT);

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);

    model.materials[0].shader = GetGBufferShader();
    model.materials[1].shader = GetGBufferShader();

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(model, (Vector3){0, 0, 0}, 1.f, WHITE);

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
