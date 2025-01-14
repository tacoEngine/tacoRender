// tacoRender (c) Nikolas Wipper 2023-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <tacoRender.h>
#include <tr_effects.h>

#include <rlgl.h>

int main() {
    SetTargetFPS(60);

    const int screenWidth = 1000, screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "tacoRender skybox");
    Init3D();

    Camera3D camera = (Camera3D) {
            .position = (Vector3) {1, 1, 1}, .target = (Vector3) {0, 0, 0}, .up = (Vector3) {
                    0, 1, 0
            }, .fovy = 72.f, .projection = CAMERA_PERSPECTIVE
    };

    GBuffers buffers = LoadGBuffers(screenWidth, screenHeight);
    GBufferPresenter presenter = LoadPresenter(buffers);

    Skybox skybox = LoadSkyboxImage(GenImageCellular(300, 400, 10));

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        { // Render to GBuffers
            BeginGBufferMode(buffers);

            ClearBackground(BLACK);

            BeginMode3D(camera);

            DrawSkybox(skybox, WHITE);

            EndMode3D();

            EndGBufferMode();
        }

        ClearPresenter(presenter);
        ShadeFlat(presenter);

        // Render GBuffers to screen
        Present(presenter);
    }

    Uninit3D();
    CloseWindow();
}
