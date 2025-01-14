// tacoRender (c) Nikolas Wipper 2024-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <tacoRender.h>
#include <tr_effects.h>

#include <cmath>

#include "raymath.h"

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "tacoRender lit sphere");
    Init3D();

    Camera3D camera = (Camera3D) {
            .position = (Vector3) {0, 0, -5}, .target = (Vector3) {0, 0, 0}, .up = (Vector3) {
                    0, 1, 0
            }, .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
    };

    Texture albedo = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
    Texture normal = LoadTextureFromImage(GenImageColor(1, 1, (Color) {128, 128, 255, 255}));
    Texture height = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture metallic = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture roughness = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
    Texture emission = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture ao = LoadTextureFromImage(GenImageWhiteNoise(500, 500, 0.99));

    Model monkey = LoadModel("examples/assets/model/suzanne.obj");
    monkey.materials[0].shader = GetGBufferShader();
    monkey.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    monkey.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    monkey.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    monkey.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    monkey.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    monkey.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    monkey.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);

    DisableCursor();

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(monkey, (Vector3) {0, 0, 0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        ClearPresenter(presenter);

        ApplySSAO(presenter, camera);

        { // Shade scene
            BeginLightingPass(presenter);

            ClearBackground(BLACK);

            EndLightingPass();
        }

        BeginDrawing();

        ClearBackground(BLANK);

        DrawTexture(presenter.source.ao, 0, 0, WHITE);

        EndDrawing();
    }

    Uninit3D();
    CloseWindow();
}
