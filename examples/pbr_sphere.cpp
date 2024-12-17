// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <rl3d.h>
#include <rl3d_effects.h>

#include <rlgl.h>

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "rl3d emissions sphere");
    Init3D();

    Camera3D camera = (Camera3D) {
            .position = (Vector3) {0, 2, -5}, .target = (Vector3) {0, 0, 0}, .up = (Vector3) {
                    0, 1, 0
            }, .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
    };

    Texture albedo = LoadTexture("examples/assets/texture/albedo.png");
    Texture normal = LoadTexture("examples/assets/texture/normals.png");
    Texture height = LoadTexture("examples/assets/texture/height.png");
    Texture metallic = LoadTexture("examples/assets/texture/metallic.png");
    Texture roughness = LoadTexture("examples/assets/texture/roughness.png");
    Texture emission = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture ao = LoadTexture("examples/assets/texture/ao.png");

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
        UpdateCamera(&camera, CAMERA_ORBITAL);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(sphere, (Vector3) {0, 0, 0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        ClearPresenter(presenter);

        { // Shade scene
            BeginLightingPass(presenter);

            ClearBackground(BLACK);

            LightSun(presenter, camera, (Vector3) {0, -1, -1}, 1, WHITE, NULL_SHADOW_MAP);

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
