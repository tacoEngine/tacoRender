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
#include <raymath.h>

void RunLightShaderEx(GBufferPresenter presenter, Camera camera, Shader shader, TextureCubemap prefilter,
                      TextureCubemap irradiance, Texture brdf) {
    static Model plane = {0};
    Camera topdown = (Camera) {
        (Vector3) {0, 1, 0},
        (Vector3) {0, 0, 0},
        (Vector3) {0, 0, -1},
        1,
        CAMERA_ORTHOGRAPHIC
    };
    if (plane.meshes == nullptr) {
        plane = LoadModelFromMesh(GenMeshPlane(1, 1, 1, 1));
    }

    int invViewLoc = GetShaderLocation(shader, "invView");
    int invProjLoc = GetShaderLocation(shader, "invProj");

    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position, SHADER_UNIFORM_VEC3);
    double top = 0.01 * tan(camera.fovy * 0.5 * DEG2RAD);
    double right = top * ((float) GetScreenWidth() / (float) GetScreenHeight());
    Matrix projection = MatrixFrustum(-right, right, -top, top, 0.01, 1000.0);

    SetShaderValueMatrix(shader, invViewLoc, MatrixInvert(GetCameraMatrix(camera)));
    SetShaderValueMatrix(shader, invProjLoc, MatrixInvert(projection));

    plane.materials[0].shader = shader;

    plane.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = presenter.source.albedo;
    plane.materials[0].maps[MATERIAL_MAP_METALNESS].texture = presenter.source.metallic;
    plane.materials[0].maps[MATERIAL_MAP_NORMAL].texture = presenter.source.normal;
    plane.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = presenter.source.roughness;
    plane.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = presenter.source.ao;
    plane.materials[0].maps[MATERIAL_MAP_EMISSION].texture = presenter.source.emission;
    plane.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = presenter.source.depth;
    plane.materials[0].maps[MATERIAL_MAP_PREFILTER].texture = prefilter;
    plane.materials[0].maps[MATERIAL_MAP_IRRADIANCE].texture = irradiance;
    plane.materials[0].maps[MATERIAL_MAP_BRDF].texture = brdf;

    float aspect = (float) GetScreenWidth() / (float) GetScreenHeight();

    BeginMode3D(topdown);

    DrawModelEx(plane, (Vector3) {0, 0, 0}, (Vector3) {0, 0, 0}, 0, (Vector3) {aspect, 1, 1}, WHITE);

    EndMode3D();
}

void RunLightShader(GBufferPresenter presenter, Camera camera, Shader shader) {
    RunLightShaderEx(presenter, camera, shader, (Texture) {0}, (Texture) {0}, (Texture) {0});
}

void RunPostProcessShader(GBufferPresenter presenter, Shader shader) {
    BeginTextureMode(presenter.back[0]);

    BeginShaderMode(shader);

    DrawTexture(presenter.target.texture, 0, 0, WHITE);

    EndShaderMode();

    EndTextureMode();

    BeginTextureMode(presenter.target);

    DrawTexture(presenter.back[0].texture, 0, 0, WHITE);

    EndTextureMode();
}

void BeginLightingPass(GBufferPresenter presenter) {
    rlEnableColorBlend();

    BeginTextureMode(presenter.target);

    BeginBlendMode(BLEND_ADD_COLORS);
}

void EndLightingPass() {
    EndBlendMode();

    EndTextureMode();

    rlDisableColorBlend();
}

Skybox LoadSkybox(const char *filename) {
    return LoadSkyboxImage(LoadImage(filename));
}

Skybox LoadSkyboxImage(Image image) {
    float vertices[] = {
        -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    };

    Mesh mesh = (Mesh) {0};
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
    // no need to glDepthFunc(GL_LEQUAL) because that's what rlgl uses by default
    DrawModel(skybox.box, (Vector3) {0, 0, 0}, 1, tint);
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

        SetShaderValue(horizontal ? bloomShaderHorizontal : bloomShaderVertical,
                       horizontal ? horRadiusLoc : vertRadiusLoc, &radius, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(horizontal ? bloomShaderHorizontal : bloomShaderVertical);
        DrawTexture(*tex, 0, 0, WHITE);
        EndTextureMode();

        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;

        EndTextureMode();
    }

    EndShaderMode();

    AddBackBuffer(presenter);
}

void ApplyGammaCorrection(GBufferPresenter presenter, float gamma) {
    static Shader gammaShader = {0};
    static int gammaLoc;
    if (gammaShader.id == 0) {
        gammaShader = GetShader(SHADER_GAMMA);
        gammaLoc = GetShaderLocation(gammaShader, "gamma");
    }

    SetShaderValue(gammaShader, gammaLoc, &gamma, SHADER_UNIFORM_FLOAT);

    RunPostProcessShader(presenter, gammaShader);
}

void ApplyToneMapping(GBufferPresenter presenter, ToneMapper mapper) {
    switch (mapper) {
    case TONE_MAP_REINHARD:
        return RunPostProcessShader(presenter, GetShader(SHADER_TONE_MAP_REINHARD));
    }
}

void LightPoint(GBufferPresenter presenter, Camera camera, Vector3 position, float intensity, float radius, Color tint) {
    static Shader point = {0};
    static int posLoc, intensityLoc, radiusLoc, colorLoc;
    if (point.id == 0) {
        point = GetShader(SHADER_POINT);
        posLoc = GetShaderLocation(point, "pos");
        intensityLoc = GetShaderLocation(point, "intensity");
        radiusLoc = GetShaderLocation(point, "radius");
        colorLoc = GetShaderLocation(point, "color");
    }
    SetShaderValue(point, posLoc, &position, SHADER_UNIFORM_VEC3);
    SetShaderValue(point, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(point, radiusLoc, &radius, SHADER_UNIFORM_FLOAT);
    Vector3 color = {(float) tint.r / 255.f, (float) tint.g / 255.f, (float) tint.b / 255.f};
    SetShaderValue(point, colorLoc, &color, SHADER_UNIFORM_VEC3);

    RunLightShader(presenter, camera, point);
}

void LightSun(GBufferPresenter presenter, Camera camera, Vector3 direction, float intensity, Color tint) {
    static Shader sun = {0};
    static int dirLoc, intensityLoc, colorLoc;
    if (sun.id == 0) {
        sun = GetShader(SHADER_SUN);
        dirLoc = GetShaderLocation(sun, "direction");
        intensityLoc = GetShaderLocation(sun, "intensity");
        colorLoc = GetShaderLocation(sun, "color");
    }

    direction = Vector3Normalize(direction);

    SetShaderValue(sun, dirLoc, &direction, SHADER_UNIFORM_VEC3);
    SetShaderValue(sun, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    Vector3 color = {(float) tint.r / 255.f, (float) tint.g / 255.f, (float) tint.b / 255.f};
    SetShaderValue(sun, colorLoc, &color, SHADER_UNIFORM_VEC3);

    RunLightShader(presenter, camera, sun);
}

const char rl3d_brdf_lut[] = {
#embed "assets/brdf.png"
    , '\0'
};

void LightIBL(GBufferPresenter presenter, Camera camera, TextureCubemap radiance, TextureCubemap irradiance) {
    static Shader ibl = {0};
    static Texture brdf;
    if (ibl.id == 0) {
        ibl = GetShader(SHADER_IBL);
        brdf = LoadTextureFromImage(LoadImageFromMemory(".png", rl3d_brdf_lut, sizeof(rl3d_brdf_lut)));
    }

    RunLightShaderEx(presenter, camera, ibl, radiance, irradiance, brdf);
}
