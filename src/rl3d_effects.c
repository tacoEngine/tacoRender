// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "rl3d_effects.h"

#include "rl3d.h"
#include "rl3d_shaders.h"

#include <rlgl.h>

Skybox LoadSkybox(const char *filename) {
    return LoadSkyboxImage(LoadImage(filename));
}

Skybox LoadSkyboxImage(Image image) {
    float vertices[] = {
            -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    };

    Mesh mesh = (Mesh){0};
    mesh.vertices = vertices;
    mesh.vertexCount = sizeof(vertices) / sizeof(float) / 3;
    mesh.triangleCount = mesh.vertexCount / 3;

    UploadMesh(&mesh, false);

    Model skybox = LoadModelFromMesh(mesh);
    Texture cubemap = LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);

    skybox.materials[0].shader = GetShader(SHADER_SKYBOX);
    skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = cubemap;

    return (Skybox) {
            skybox
    };
}

void DrawSkybox(Skybox skybox, Color tint) {
    rlDisableDepthMask();

    DrawModel(skybox.box, (Vector3){0, 0, 0}, 1, tint);

    rlEnableDepthMask();
}

void ShadeFlat(GBufferPresenter presenter) {
    BeginTextureMode(presenter.target);

    DrawTexture(presenter.source.albedo, 0, 0, WHITE);

    EndTextureMode();
}

void ApplyBloom(GBufferPresenter presenter, unsigned int iterations, float radius) {
    static Shader bloomShaderHorizontal = {0};
    static Shader bloomShaderVertical = {0};
    static int horRadiusLoc, vertRadiusLoc;
    if (bloomShaderHorizontal.id == 0) {
        bloomShaderHorizontal = GetShader(SHADER_BLUR_HOR);
        bloomShaderVertical = GetShader(SHADER_BLUR_VERT);
        horRadiusLoc = GetShaderLocation(bloomShaderHorizontal, "radius");
        vertRadiusLoc = GetShaderLocation(bloomShaderVertical, "radius");
    }

    bool horizontal = true, first_iteration = true;

    for (unsigned int i = 0; i < iterations * 2; i++) {
        BeginTextureMode(presenter.back[horizontal]);
        rlClearScreenBuffers();
        Texture *tex = first_iteration ? &presenter.source.emission : &presenter.back[!horizontal].texture;

        SetShaderValue(horizontal ? bloomShaderHorizontal : bloomShaderVertical, horizontal ? horRadiusLoc : vertRadiusLoc, &radius, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(horizontal ? bloomShaderHorizontal : bloomShaderVertical);
        DrawTexture(*tex, 0, 0, WHITE);
        EndTextureMode();

        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;

        EndTextureMode();
    }

    EndShaderMode();

    AddBackBuffer(presenter, !horizontal);
}
