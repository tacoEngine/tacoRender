// tacoRender (c) Nikolas Wipper 2024-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cstdio>
#include <tacoRender.h>
#include <tr_effects.h>
#include <raymath.h>

#include "tr_shaders.h"

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "tacoRender sss");
    Init3D();

    Camera3D camera = (Camera3D) {
        .position = (Vector3) {-2, 4, -8},
        .target = (Vector3) {0., 0.5, 0},
        .up = (Vector3) {
            0,
            1,
            0
        },
        .fovy = 72.f,
        .projection = CAMERA_PERSPECTIVE
    };

    Texture albedo = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
    Texture normal = LoadTextureFromImage(GenImageColor(1, 1, (Color) {128, 128, 255, 255}));
    Texture height = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture metallic = LoadTextureFromImage(GenImageColor(1, 1, GRAY));
    Texture roughness = LoadTextureFromImage(GenImageColor(1, 1, GRAY));
    Texture emission = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    Texture ao = LoadTextureFromImage(GenImageColor(1, 1, WHITE));

    Model model = LoadModel("examples/assets/model/testscene.obj");
    model.materials[0].shader = GetGBufferShader();
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    model.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    model.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    model.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    model.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    model.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    Model light = LoadModelFromMesh(GenMeshSphere(.1f, 10, 10));
    light.materials[0].shader = GetGBufferShader();
    light.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = albedo;
    light.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normal;
    light.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = height;
    light.materials[0].maps[MATERIAL_MAP_METALNESS].texture = metallic;
    light.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    light.materials[0].maps[MATERIAL_MAP_EMISSION].texture = emission;
    light.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ao;

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);
    ScreenShadowMap shadowMap = LoadScreenShadowMap(screenWidth, screenHeight);

    Vector3 light_position = {3, 4, 0};

    DisableCursor();
    SetExitKey(0);

    enum draw_mode {shaded, unshaded, depth, shadow} draw_mode = shaded;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE))
            draw_mode = shaded;
        if (IsKeyPressed(KEY_TWO))
            draw_mode = unshaded;
        if (IsKeyPressed(KEY_THREE))
            draw_mode = depth;
        if (IsKeyPressed(KEY_FOUR))
            draw_mode = shadow;

        if (IsKeyPressed(KEY_ESCAPE))
            EnableCursor();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            DisableCursor();

        if (IsCursorHidden())
            UpdateCamera(&camera, CAMERA_FREE);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawModel(model, (Vector3) {0, 0, 0}, 1, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        ClearPresenter(presenter);

        { // Shade scene
            BeginLightingPass(presenter);

            ClearBackground(BLACK);

            LightPoint(presenter, camera, light_position, 10, 100, WHITE, shadowMap);

            EndLightingPass();
        }

        ComputeScreenShadowMap(presenter, shadowMap, camera, light_position);

        ApplyGammaCorrection(presenter, 2.2);

        BeginDrawing();

        ClearBackground(BLANK);

        if (draw_mode == shaded)
            DrawTexture(presenter.target.texture, 0, 0, WHITE);
        else if (draw_mode == unshaded) {
            BeginShaderMode(GetShader(SHADER_FLIP_Y));
            DrawTexture(presenter.source.albedo, 0, 0, WHITE);
            EndShaderMode();
        } else if (draw_mode == depth)
            DrawDepth(presenter.source.depth, Vector2 {0, 0}, 0, 1, WHITE);
        else if (draw_mode == shadow)
            DrawDepth(shadowMap.back[0].texture, Vector2 {0, 0}, 0, 1, WHITE);

        EndDrawing();
    }

    //Uninit3D();
    CloseWindow();
}
