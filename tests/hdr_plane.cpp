// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <rl3d.h>
#include <rl3d_effects.h>

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "rl3d hdr plane");
    Init3D();

    Camera3D camera = (Camera3D) {
            .position = (Vector3) {2, 3, 5}, .target = (Vector3) {0, 0, 0}, .up = (Vector3) {
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

    Model plane = LoadModelFromMesh(GenMeshPlane(10, 10, 1, 1));
    plane.materials[0].shader = GetGBufferShader();
    plane.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    plane.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    plane.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    plane.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    plane.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    plane.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    plane.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);

    bool map = true;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) map = !map;

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLANK);

            BeginMode3D(camera);

            DrawModel(plane, (Vector3) {0, 0, 0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        ClearPresenter(presenter);

        LightPoint(presenter, camera, (Vector3) {-3, 0.5, 0}, 1, DARKGRAY);
        LightPoint(presenter, camera, (Vector3) {-1, 0.5, 0}, 1, GRAY);
        LightPoint(presenter, camera, (Vector3) { 3, 0.5, 0}, 1, LIGHTGRAY);
        LightPoint(presenter, camera, (Vector3) { 1, 0.5, 0}, 1, WHITE);

        AddBackBuffer(presenter);

        if (map) {
            ApplyToneMapping(presenter, TONE_MAP_REINHARD);
        }

        ApplyGammaCorrection(presenter, 2.2);

        // Render GBuffers to screen
        Present(presenter);
    }

    Uninit3D();
    CloseWindow();
}
