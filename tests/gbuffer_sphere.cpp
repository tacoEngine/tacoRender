// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <rl3d.h>
#include <raylib.h>

int main() {
    SetTargetFPS(60);

    InitWindow(500, 500, "rl3d_gbuffer_sphere");
    Init3D();

    Camera3D camera = (Camera3D) {
        .position = (Vector3) {0, 0, -2}, .target = (Vector3) {0, 0, 0}, .up = (Vector3) {
            0, 1, 0
        }, .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
    };

    Texture albedo = LoadTextureFromImage(GenImageColor(1, 1, RED));
    Texture normal = LoadTextureFromImage(GenImageColor(1, 1, (Color) {128, 128, 255, 255}));
    Texture height = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture metallic = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture roughness = LoadTextureFromImage(GenImageColor(1, 1, GRAY));
    Texture emission = LoadTextureFromImage(GenImageColor(1, 1, BLANK));
    Texture ao = LoadTextureFromImage(GenImageColor(1, 1, WHITE));

    Model sphere = LoadModelFromMesh(GenMeshSphere(1, 10, 30));
    sphere.materials[0].shader = GetGBufferShader();
    sphere.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    sphere.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    sphere.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    sphere.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    sphere.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    sphere.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    sphere.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    GBuffers buffers = LoadGBuffers(500, 500);

    while (!WindowShouldClose()) {
        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(sphere, (Vector3){0,0,0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        { // Render GBuffers to screen
            BeginDrawing();

            ClearBackground(BLACK);

            DrawTextureEx(buffers.albedo, (Vector2){0,0}, 0, 0.3333, WHITE);
            DrawTextureEx(buffers.normal, (Vector2){166,0}, 0, 0.3333, WHITE);
            DrawTextureEx(buffers.height, (Vector2){333,0}, 0, 0.3333, WHITE);

            DrawTextureEx(buffers.metallic, (Vector2){0,166}, 0, 0.3333, WHITE);
            DrawTextureEx(buffers.roughness, (Vector2){166,166}, 0, 0.3333, WHITE);
            DrawTextureEx(buffers.emission, (Vector2){333,166}, 0, 0.3333, WHITE);

            DrawTextureEx(buffers.ao, (Vector2){0,333}, 0, 0.3333, WHITE);
            DrawDepth(buffers.depth, (Vector2){166,333}, 0, 0.3333, WHITE);
            DrawRectangle(333, 333, 166, 166, BLACK);

            EndDrawing();
        }
    }

    Uninit3D();
    CloseWindow();
}
