// rl3d (c) Nikolas Wipper 2024

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cstdio>
#include <rl3d.h>
#include <rl3d_effects.h>
#include <raymath.h>

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "rl3d shadowmap");
    Init3D();

    Camera3D camera = (Camera3D) {
        .position = (Vector3) {-2, 0, 0}, .target = (Vector3) {0, 0, 0}, .up = (Vector3) {
            0, 1, 0
        },
        .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
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

    Model box = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    box.materials[0].shader = GetGBufferShader();
    box.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    box.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    box.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    box.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    box.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    box.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    box.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    Model plane = LoadModelFromMesh(GenMeshCube(100, 1, 100));
    plane.materials[0].shader = GetGBufferShader();
    plane.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    plane.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    plane.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    plane.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    plane.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    plane.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    plane.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    box.transform = MatrixRotateY(45);

    const int CASCADE_COUNT = 3;

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);
    ShadowMap shadow_map = LoadShadowMap(2048, CASCADE_COUNT);

    DisableCursor();

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(sphere, (Vector3) {0, 0, 0}, 1, WHITE);
            DrawModel(box, (Vector3) {0, 0, 4}, 1, WHITE);
            DrawModel(plane, (Vector3) {0, -1, 0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        for (int i = 0; i < CASCADE_COUNT; i++) {
            BeginShadowMap(shadow_map, camera, (Vector3) {1, -1, 0}, i);

            ClearBackground(BLANK);

            DrawModel(sphere, (Vector3) {0, 0, 0}, 1, WHITE);
            DrawModel(box, (Vector3) {0, 0, 4}, 1, WHITE);
            DrawModel(plane, (Vector3) {0, -1, 0}, 1, WHITE);

            EndShadowMap();
        }

        ClearPresenter(presenter);

        { // Shade scene
            BeginLightingPass(presenter);

            ClearBackground(BLACK);

            LightSun(presenter, camera, (Vector3) {1, -1, 0}, 1, YELLOW, shadow_map);

            EndLightingPass();
        }

        ApplyGammaCorrection(presenter, 2.2);

        BeginDrawing();

        ClearBackground(BLANK);

        DrawTexture(presenter.target.texture, 0, 0, WHITE);

        EndDrawing();
    }

    //Uninit3D();
    CloseWindow();
}
