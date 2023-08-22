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

#include <stddef.h>

void DrawMeshWithGBuffers(GBufferPresenter presenter, Mesh mesh, Shader shader) {
    // Bind shader program
    rlEnableShader(shader.id);

    // Send required data to shader (matrices, values)
    //-----------------------------------------------------

    // Get a copy of current matrices to work with,
    // just in case stereo render is required, and we need to modify them
    // NOTE: At this point the modelview matrix just contains the view matrix (camera)
    // That's because BeginMode3D() sets it and there is no model-drawing function
    // that modifies it, all use rlPushMatrix() and rlPopMatrix()
    Matrix matModel = MatrixIdentity();
    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();

    // Upload view and projection matrices (if locations available)
    if (shader.locs[SHADER_LOC_MATRIX_VIEW] != -1) rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], matView);
    if (shader.locs[SHADER_LOC_MATRIX_PROJECTION] != -1)
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);

    // Model transformation matrix is sent to shader uniform location: SHADER_LOC_MATRIX_MODEL
    if (shader.locs[SHADER_LOC_MATRIX_MODEL] != -1)
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_MODEL], MatrixIdentity());

    // Upload model normal matrix (if locations available)
    if (shader.locs[SHADER_LOC_MATRIX_NORMAL] != -1)
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));
    //-----------------------------------------------------

#define GBUFFER_TEXTURE_COUNT 8
    Texture textures[GBUFFER_TEXTURE_COUNT] = {
            presenter.source.albedo,
            presenter.source.normal,
            presenter.source.height,
            presenter.source.metallic,
            presenter.source.roughness,
            presenter.source.emission,
            presenter.source.ao,
            presenter.source.depth,
    };

    int locs[GBUFFER_TEXTURE_COUNT] = {
            shader.locs[SHADER_LOC_MAP_ALBEDO],
            shader.locs[SHADER_LOC_MAP_NORMAL],
            shader.locs[SHADER_LOC_MAP_HEIGHT],
            shader.locs[SHADER_LOC_MAP_METALNESS],
            shader.locs[SHADER_LOC_MAP_ROUGHNESS],
            shader.locs[SHADER_LOC_MAP_EMISSION],
            shader.locs[SHADER_LOC_MAP_OCCLUSION],
            GetShaderLocation(shader, "depth"),
    };

    // Bind active texture maps (if available)
    for (int i = 0; i < GBUFFER_TEXTURE_COUNT; i++) {
        if (textures[i].id > 0) {
            // Select current shader texture slot
            rlActiveTextureSlot(i);

            // Enable texture for active slot
            // once we have cubemaps, use rlEnableTextureCubemap();
            rlEnableTexture(textures[i].id);
            rlSetUniform(locs[i], &i, SHADER_UNIFORM_INT, 1);
        }
    }

    rlEnableVertexArray(mesh.vaoId);

    // WARNING: Disable vertex attribute color input if mesh can not provide that data (despite location being enabled in shader)
    if (mesh.vboId[3] == 0) rlDisableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_COLOR]);

    int eyeCount = 1;
    if (rlIsStereoRenderEnabled()) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++) {
        // Calculate model-view-projection matrix (MVP)
        Matrix matModelViewProjection;
        if (eyeCount == 1) matModelViewProjection = MatrixMultiply(matView, matProjection);
        else {
            // Setup current eye viewport (half screen width)
            rlViewport(eye * rlGetFramebufferWidth() / 2, 0, rlGetFramebufferWidth() / 2, rlGetFramebufferHeight());
            matModelViewProjection = MatrixMultiply(MatrixMultiply(matView, rlGetMatrixViewOffsetStereo(eye)),
                                                    rlGetMatrixProjectionStereo(eye));
        }

        // Send combined model-view-projection matrix to shader
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

        // Draw mesh
        if (mesh.indices != NULL) rlDrawVertexArrayElements(0, mesh.triangleCount * 3, 0);
        else rlDrawVertexArray(0, mesh.vertexCount);
    }

    // Unbind all bound texture maps
    for (int i = 0; i < GBUFFER_TEXTURE_COUNT; i++) {
        if (textures[i].id > 0) {
            // Select current shader texture slot
            rlActiveTextureSlot(i);

            // Disable texture for active slot
            // once we have cubemaps, use rlDisableTextureCubemap();
            rlDisableTexture();
        }
    }

    // Disable all possible vertex array objects (or VBOs)
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();

    // Disable shader program
    rlDisableShader();

    // Restore rlgl internal modelview and projection matrices
    rlSetMatrixModelview(matView);
    rlSetMatrixProjection(matProjection);
}

void RunLightShader(GBufferPresenter presenter, Camera camera, Shader shader) {
    static Mesh plane = {0};
    Camera topdown = (Camera) {
            (Vector3) {0, 1, 0},
            (Vector3) {0, 0, 0},
            (Vector3) {0, 0, -1},
            1,
            CAMERA_ORTHOGRAPHIC
    };
    if (plane.vboId == 0) {
        plane = GenMeshPlane(1, 1, 1, 1);
    }

    int invViewLoc = GetShaderLocation(shader, "invView");
    int invProjLoc = GetShaderLocation(shader, "invProj");

    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position, SHADER_UNIFORM_VEC3);
    double top = 0.01 * tan(camera.fovy * 0.5 * DEG2RAD);
    double right = top * ((float) GetScreenWidth() / (float) GetScreenHeight());
    Matrix projection = MatrixFrustum(-right, right, -top, top, 0.01, 1000.0);

    SetShaderValueMatrix(shader, invViewLoc, MatrixInvert(GetCameraMatrix(camera)));
    SetShaderValueMatrix(shader, invProjLoc, MatrixInvert(projection));

    BeginMode3D(topdown);

    DrawMeshWithGBuffers(presenter, plane, shader);

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
