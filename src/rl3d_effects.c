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

void RunLightShader(GBufferPresenter presenter, Camera camera, Shader shader) {
    static Model plane = {0};
    Camera topdown = (Camera) {
            (Vector3) {0, 1, 0},
            (Vector3) {0, 0, 0},
            (Vector3) {0, 0, -1},
            1,
            CAMERA_ORTHOGRAPHIC
    };
    if (plane.meshes == 0) {
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

    float aspect = (float) GetScreenWidth() / (float) GetScreenHeight();

    BeginMode3D(topdown);

    DrawModelEx(plane, (Vector3){0,0,0}, (Vector3){0,0,0}, 0, (Vector3){aspect,1,1}, WHITE);

    EndMode3D();
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

void LightPoint(GBufferPresenter presenter, Camera camera, Vector3 position, float intensity, Color tint) {
    static Shader pointPhong = {0};
    static int posLoc, intensityLoc, radiusLoc, colorLoc;
    if (pointPhong.id == 0) {
        pointPhong = GetShader(SHADER_POINT);
        posLoc = GetShaderLocation(pointPhong, "pos");
        intensityLoc = GetShaderLocation(pointPhong, "intensity");
        radiusLoc = GetShaderLocation(pointPhong, "radius");
        colorLoc = GetShaderLocation(pointPhong, "color");
    }
    // finalColor = intensity * (1 / (lightDistance * lightDistance)) * (diff * color + specular) * color;
    // so the worst case (brightest point) is
    // finalColor = intensity * (1 / (lightDistance * lightDistance)) * (1 * vec4(1) + 1) * vec4(1);
    // finalColor = intensity * (1 / (lightDistance * lightDistance)) * vec4(2);
    // this becomes vec4(vec3(0), 1) when it goes below 1/255 for 32bpp rgba, so bc r, g, b and a are equal
    // 1/255 = intensity * (1 / (lightDistance * lightDistance)) * 2
    // 1/510 = intensity * (1 / (lightDistance * lightDistance))
    // 1/(510 * intensity) = 1 / (lightDistance * lightDistance)
    // 510 * intensity = lightDistance * lightDistance
    // in the shader we then compare with lightDistance * lightDistance,
    // so we don't have to take the square root in here every time
    float radius = 510 * intensity;
    SetShaderValue(pointPhong, posLoc, &position, SHADER_UNIFORM_VEC3);
    SetShaderValue(pointPhong, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pointPhong, radiusLoc, &radius, SHADER_UNIFORM_FLOAT);
    Vector4 color = {(float) tint.r / 255.f, (float) tint.g / 255.f, (float) tint.b / 255.f, (float) tint.a / 255.f};
    SetShaderValue(pointPhong, colorLoc, &color, SHADER_UNIFORM_VEC4);

    RunLightShader(presenter, camera, pointPhong);
}

void LightSun(GBufferPresenter presenter, Camera camera, Vector3 direction, float intensity, Color tint) {
    static Shader sunPhong = {0};
    static int dirLoc, intensityLoc, colorLoc;
    if (sunPhong.id == 0) {
        sunPhong = GetShader(SHADER_SUN);
        dirLoc = GetShaderLocation(sunPhong, "direction");
        intensityLoc = GetShaderLocation(sunPhong, "intensity");
        colorLoc = GetShaderLocation(sunPhong, "color");
    }

    direction = Vector3Normalize(direction);

    SetShaderValue(sunPhong, dirLoc, &direction, SHADER_UNIFORM_VEC3);
    SetShaderValue(sunPhong, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    Vector4 color = {(float) tint.r / 255.f, (float) tint.g / 255.f, (float) tint.b / 255.f, (float) tint.a / 255.f};
    SetShaderValue(sunPhong, colorLoc, &color, SHADER_UNIFORM_VEC4);

    RunLightShader(presenter, camera, sunPhong);
}
