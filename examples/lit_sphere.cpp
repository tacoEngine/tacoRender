// tacoRender (c) Nikolas Wipper 2023-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <tacoRender.h>
#include <tr_effects.h>

#include <cmath>

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

    Texture albedo = LoadTextureFromImage(GenImageColor(1, 1, BLUE));
    Texture normal = LoadTextureFromImage(GenImageColor(1, 1, (Color) {128, 128, 255, 255}));
    Texture height = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture metallic = LoadTextureFromImage(GenImageColor(1, 1, GRAY));
    Texture roughness = LoadTextureFromImage(GenImageColor(1, 1, GRAY));
    Texture emission = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture ao = LoadTextureFromImage(GenImageColor(1, 1, WHITE));

    Model sphere = LoadModelFromMesh(GenMeshSphere(1, 50, 50));
    sphere.materials[0].shader = GetGBufferShader();
    sphere.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    sphere.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    sphere.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    sphere.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    sphere.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    sphere.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    sphere.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);

    while (!WindowShouldClose()) {
        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(sphere, (Vector3) {0, 0, 0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        auto time = (float) GetTime();

        ClearPresenter(presenter);

        { // Shade scene
            BeginLightingPass(presenter);

            ClearBackground(BLACK);

            LightSun(presenter, camera, (Vector3) {1, -1, 0}, 1, YELLOW, NULL_SHADOW_MAP);
            LightPoint(presenter, camera, (Vector3) {2 * sinf(time), 2 * cosf(time), -2}, 2, 2 * 510, WHITE);

            EndLightingPass();
        }

        ApplyGammaCorrection(presenter, 2.2);

        Present(presenter);
    }

    Uninit3D();
    CloseWindow();
}
