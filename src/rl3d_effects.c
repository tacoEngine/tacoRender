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
#include <external/glad.h>

#include <stdlib.h>

#define BindTextureSlot(slot, id, loc) if (id >= 0 && loc >= 0) {\
rlActiveTextureSlot(slot); \
rlEnableTexture(id); \
int s = slot; \
rlSetUniform(loc, &s, SHADER_UNIFORM_INT, 1); \
}

#define UnbindTextureSlot(slot) {\
rlActiveTextureSlot(slot); \
rlDisableTexture(); \
}

#define BindTextureSlotCubemap(slot, id, loc) {\
rlActiveTextureSlot(slot); \
rlEnableTextureCubemap(id); \
int s = slot; \
rlSetUniform(loc, &s, SHADER_UNIFORM_INT, 1); \
}

#define UnbindTextureSlotCubemap(slot) {\
rlActiveTextureSlot(slot); \
rlDisableTextureCubemap(); \
}

void RunLightShaderPro(GBufferPresenter presenter, Camera camera, Shader shader, int cubemapCount,
                       unsigned int *cubemapIDs, int *cubemapLocs, int extraTextureCount, unsigned int *extraTextureIDs,
                       int *extraTextureLocs) {
    static Mesh plane = {0};
    Camera topdown = (Camera) {
        (Vector3) {0, 1, 0},
        (Vector3) {0, 0, 0},
        (Vector3) {0, 0, -1},
        1,
        CAMERA_ORTHOGRAPHIC
    };
    if (plane.vaoId == 0) {
        plane = GenMeshPlane(1, 1, 1, 1);
    }

    int invViewLoc = GetShaderLocation(shader, "invView");
    int invProjLoc = GetShaderLocation(shader, "invProj");
    int camViewLoc = GetShaderLocation(shader, "camView");
    int camProjLoc = GetShaderLocation(shader, "camProj");

    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position, SHADER_UNIFORM_VEC3);
    double top = 0.01 * tan(camera.fovy * 0.5 * DEG2RAD);
    double right = top * ((float) GetScreenWidth() / (float) GetScreenHeight());
    Matrix projection = MatrixFrustum(-right, right, -top, top, 0.01, 1000.0);

    SetShaderValueMatrix(shader, invViewLoc, MatrixInvert(GetCameraMatrix(camera)));
    SetShaderValueMatrix(shader, invProjLoc, MatrixInvert(projection));
    SetShaderValueMatrix(shader, camViewLoc, GetCameraMatrix(camera));
    SetShaderValueMatrix(shader, camProjLoc, projection);

    float aspect = (float) GetScreenWidth() / (float) GetScreenHeight();

    BeginMode3D(topdown);

    // Calculate transformation matrix from function parameters
    // Get transform matrix (rotation -> scale -> translation)
    Matrix matScale = MatrixScale(aspect, 1, 1);

    // Bind shader program
    rlEnableShader(shader.id);

    // Bind active texture maps (if available)
    BindTextureSlot(0, presenter.source.albedo.id, shader.locs[SHADER_LOC_MAP_ALBEDO])
    BindTextureSlot(1, presenter.source.metallic.id, shader.locs[SHADER_LOC_MAP_METALNESS])
    BindTextureSlot(2, presenter.source.normal.id, shader.locs[SHADER_LOC_MAP_NORMAL])
    BindTextureSlot(3, presenter.source.roughness.id, shader.locs[SHADER_LOC_MAP_ROUGHNESS])
    BindTextureSlot(4, presenter.source.ao.id, shader.locs[SHADER_LOC_MAP_OCCLUSION])
    BindTextureSlot(5, presenter.source.emission.id, shader.locs[SHADER_LOC_MAP_EMISSION])
    BindTextureSlot(6, presenter.source.depth.id, shader.locs[SHADER_LOC_MAP_HEIGHT])

    for (int i = 0; i < cubemapCount; i++) {
        BindTextureSlotCubemap(7 + i, cubemapIDs[i], cubemapLocs[i]);
    }

    for (int i = 0; i < extraTextureCount; i++) {
        BindTextureSlot(7 + cubemapCount + i, extraTextureIDs[i], extraTextureLocs[i]);
    }

    // Try binding vertex array objects (VAO) or use VBOs if not possible
    // WARNING: UploadMesh() enables all vertex attributes available in mesh and sets default attribute values
    // for shader expected vertex attributes that are not provided by the mesh (i.e. colors)
    // This could be a dangerous approach because different meshes with different shaders can enable/disable some attributes
    if (!rlEnableVertexArray(plane.vaoId)) {
        // Bind mesh VBO data: vertex position (shader-location = 0)
        rlEnableVertexBuffer(plane.vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION]);
        rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION]);

        // Bind mesh VBO data: vertex texcoords (shader-location = 1)
        rlEnableVertexBuffer(plane.vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD]);
        rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_TEXCOORD01], 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_TEXCOORD01]);

        if (plane.indices != NULL) rlEnableVertexBufferElement(plane.vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES]);
    }

    int eyeCount = 1;
    if (rlIsStereoRenderEnabled()) eyeCount = 2;

    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();

    for (int eye = 0; eye < eyeCount; eye++) {
        // Calculate model-view-projection matrix (MVP)
        Matrix matModelViewProjection = MatrixMultiply(matScale, matView);
        if (eyeCount == 1) matModelViewProjection = MatrixMultiply(matModelViewProjection, matProjection);
        else {
            // Setup current eye viewport (half screen width)
            rlViewport(eye * presenter.target.texture.width / 2,
                       0,
                       presenter.target.texture.width / 2,
                       presenter.target.texture.height);
            matModelViewProjection = MatrixMultiply(
                MatrixMultiply(matModelViewProjection, rlGetMatrixViewOffsetStereo(eye)),
                rlGetMatrixProjectionStereo(eye));
        }

        // Send combined model-view-projection matrix to shader
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

        // Draw mesh
        if (plane.indices != NULL) rlDrawVertexArrayElements(0, plane.triangleCount * 3, 0);
        else rlDrawVertexArray(0, plane.vertexCount);
    }

    // Unbind all bound texture maps
    UnbindTextureSlot(0)
    UnbindTextureSlot(1)
    UnbindTextureSlot(2)
    UnbindTextureSlot(3)
    UnbindTextureSlot(4)
    UnbindTextureSlot(5)
    UnbindTextureSlot(6)
    UnbindTextureSlot(7)

    for (int i = 0; i < cubemapCount; i++) {
        UnbindTextureSlotCubemap(7 + i);
    }

    for (int i = 0; i < extraTextureCount; i++) {
        UnbindTextureSlot(7 + cubemapCount + i);
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

    EndMode3D();
}

void RunLightShaderEx(GBufferPresenter presenter, Camera camera, Shader shader, int extraTextureCount,
                      unsigned int *extraTextureIDs, int *extraTextureLocs) {
    RunLightShaderPro(presenter,
                      camera,
                      shader,
                      0,
                      NULL,
                      NULL,
                      extraTextureCount,
                      extraTextureIDs,
                      extraTextureLocs);
}

void RunLightShader(GBufferPresenter presenter, Camera camera, Shader shader) {
    RunLightShaderEx(presenter, camera, shader, 0, NULL, NULL);
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

Texture BlurTexture(RenderTexture back[], Texture texture, int iterations) {
    static Shader blurShader = {0};
    static int horizontalLoc;
    if (blurShader.id == 0) {
        blurShader = GetShader(SHADER_BLUR_GAUSS);
        horizontalLoc = GetShaderLocation(blurShader, "horizontal");
    }

    for (unsigned int i = 0; i < iterations * 2; i++) {
        unsigned int horizontal = i & 1;
        SetShaderValue(blurShader, horizontalLoc, &horizontal, RL_SHADER_UNIFORM_UINT);

        BeginTextureMode(back[!(i & 1)]);
        rlClearScreenBuffers();
        Texture tex = (i == 0) ? texture : back[i & 1].texture;

        BeginShaderMode(blurShader);
        DrawTexture(tex, 0, 0, WHITE);
        EndTextureMode();

        EndTextureMode();
    }

    EndShaderMode();
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

#define CULL_FAR 1000.0f
#define CULL_NEAR (-1.f)


float EndOfCascade(int cascade, int cascadeCount, float cascadeDistance) {
    float x = (float) cascade / (float) cascadeCount;
    float f0;
    if (x <= 0.7)
        f0 = (x * x) / 4.9f;
    else
        f0 = 3.f * (x - 0.7f) + 0.1f;
    float f1 = powf(x, powf(0.45f * (100 / cascadeDistance), -0.5f));

    return fmaxf(f0, f1);
}

ShadowMap LoadShadowMap(int size, int cascades, float cascadeDistance) {
    ShadowMap target = {0};

    target.fbo = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.fbo > 0) {
        rlEnableFramebuffer(target.fbo);

        target.ids = RL_CALLOC(8, sizeof(unsigned int));
        target.projections = RL_CALLOC(cascades, sizeof(Matrix));
        target.dists = RL_CALLOC(cascades, sizeof(float));

        glGenTextures(cascades, target.ids);

        for (int i = 0; i < cascades; i++) {
            glBindTexture(GL_TEXTURE_2D, target.ids[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            float dist = cascadeDistance * EndOfCascade(i + 1, cascades, cascadeDistance);
            target.dists[i] = dist;
        }

        target.back[0] = LoadCustomRenderTexture(size, size, RL_PIXELFORMAT_UNCOMPRESSED_R32, true, false);
        target.back[1] = LoadCustomRenderTexture(size, size, RL_PIXELFORMAT_UNCOMPRESSED_R32, true, false);

        for (int i = cascades; i < 8; i++) {
            target.ids[i] = target.ids[cascades - 1];
        }

        target.size = size;
        target.cascades = cascades;

        // Attach color texture and depth renderbuffer/texture to FBO
        rlFramebufferAttach(target.fbo, target.ids[0], RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.fbo))
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);
        else
            TRACELOG(LOG_WARNING, "FBO: [ID %i] Framebuffer object creation incomplete", target.id);

        rlDisableFramebuffer();
    } else
        TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

void UnloadShadowMap(ShadowMap shadowMap) {
    for (int i = 0; i < shadowMap.cascades; i++) {
        rlUnloadTexture(shadowMap.ids[i]);
    }

    rlUnloadFramebuffer(shadowMap.fbo);

    RL_FREE(shadowMap.ids);
    RL_FREE(shadowMap.projections);
    RL_FREE(shadowMap.dists);
}

void BeginShadowMap(ShadowMap shadowMap, Camera camera, Vector3 lightDirection, int cascade) {
    RenderTexture dummy = (RenderTexture) {.id = shadowMap.fbo};
    dummy.texture.width = shadowMap.size;
    dummy.texture.height = shadowMap.size;

    rlFramebufferAttach(shadowMap.fbo, shadowMap.ids[cascade], RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

    BeginTextureMode(dummy);

    Matrix lightView = MatrixLookAt(Vector3Zero(), lightDirection, (Vector3) {0, 1, 0});

    Matrix camView = GetCameraMatrix(camera);
    Matrix camInv = MatrixInvert(camView);

    float previousCascade;
    if (cascade == 0)
        previousCascade = 1.f;
    else
        previousCascade = -shadowMap.dists[cascade - 1];
    float nextCascade = -shadowMap.dists[cascade];

    float aspectRatio = (float) GetScreenHeight() / (float) GetScreenWidth();
    float fovy = camera.fovy + 20;
    float tanHalfHFOV = tanf(DEG2RAD * (fovy / 2.0f));
    float tanHalfVFOV = tanf(DEG2RAD * (fovy * aspectRatio / 2.0f));

    float xn = previousCascade * tanHalfHFOV;
    float xf = nextCascade * tanHalfHFOV;
    float yn = previousCascade * tanHalfVFOV;
    float yf = nextCascade * tanHalfVFOV;

    Vector4 frustumCorners[8] = {
        // near face
        (Vector4) {xn, yn, previousCascade, 1.0f},
        (Vector4) {-xn, yn, previousCascade, 1.0f},
        (Vector4) {xn, -yn, previousCascade, 1.0f},
        (Vector4) {-xn, -yn, previousCascade, 1.0f},

        // far face
        (Vector4) {xf, yf, nextCascade, 1.0f},
        (Vector4) {-xf, yf, nextCascade, 1.0f},
        (Vector4) {xf, -yf, nextCascade, 1.0f},
        (Vector4) {-xf, -yf, nextCascade, 1.0f},
    };

    float minX = INFINITY;
    float minY = INFINITY;
    float minZ = INFINITY;

    float maxX = -INFINITY;
    float maxY = -INFINITY;
    float maxZ = -INFINITY;

    for (int j = 0; j < 8; j++) {
        // Transform the frustum coordinate from view to world space
        Vector4 worldCoordinate = QuaternionTransform(frustumCorners[j], camInv);

        // Transform the frustum coordinate from world to light space
        Vector4 lightCoordinate = QuaternionTransform(worldCoordinate, lightView);

        minX = fminf(minX, lightCoordinate.x);
        maxX = fmaxf(maxX, lightCoordinate.x);
        minY = fminf(minY, lightCoordinate.y);
        maxY = fmaxf(maxY, lightCoordinate.y);
        minZ = fminf(minZ, lightCoordinate.z);
        maxZ = fmaxf(maxZ, lightCoordinate.z);
    }

    Matrix lightProj = MatrixOrtho(minX, maxX, minY, maxY, minZ, maxZ);

    rlSetMatrixProjection(lightProj);
    rlSetMatrixModelview(lightView);

    // Calculate model-view-projection matrix (MVP)
    shadowMap.projections[cascade] = MatrixMultiply(lightView, lightProj);

    rlEnableDepthTest(); // Enable DEPTH_TEST for 3D
}

void EndShadowMap() {
    EndMode3D();
    EndTextureMode();
}

void FilterShadowMap(ShadowMap shadowMap) {
    for (int i = 0; i < shadowMap.cascades; i++) {
        Texture tex;
        tex.id = shadowMap.ids[i];
        tex.width = shadowMap.size;
        tex.height = shadowMap.size;

        rlDisableDepthTest();

        BlurTexture(shadowMap.back, tex, 2);

        // Set cascade as depth attachment
        rlFramebufferAttach(shadowMap.fbo, tex.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        RenderTexture dummy = (RenderTexture) {.id = shadowMap.fbo};
        dummy.texture.width = shadowMap.size;
        dummy.texture.height = shadowMap.size;

        rlEnableDepthTest();

        BeginTextureMode(dummy);
        rlClearScreenBuffers();
        BeginShaderMode(GetShader(SHADER_TEX_TO_DEPTH));
        DrawTexture(shadowMap.back[0].texture, 0, 0, WHITE);
        EndTextureMode();

        break;
    }
}

Skybox LoadSkybox(const char *filename) {
    return LoadSkyboxImage(LoadImage(filename));
}

Skybox LoadSkyboxImage(Image image) {
    // @formatter:off
    float vertices[] = {
        -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    };
    // @formatter:on

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

void ApplySSAO(GBufferPresenter presenter, Camera camera) {
    BeginTextureMode(presenter.back[0]);
    rlClearScreenBuffers();
    RunLightShader(presenter, camera, GetShader(SHADER_SSAO));
    EndTextureMode();

    rlEnableColorBlend();

    rlSetBlendFactorsSeparate(RL_SRC_COLOR, RL_DST_COLOR, RL_SRC_ALPHA, RL_DST_ALPHA, RL_MIN, RL_MIN);

    BeginTextureMode(presenter.occlusion);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    BeginShaderMode(GetShader(SHADER_BLUR_BOX));

    DrawTexture(presenter.back[0].texture, 0, 0, WHITE);

    EndShaderMode();
    EndBlendMode();
    EndTextureMode();
    rlDisableColorBlend();
}

void LightPoint(GBufferPresenter presenter, Camera camera, Vector3 position, float intensity, float radius,
                Color tint) {
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

void LightSun(GBufferPresenter presenter, Camera camera, Vector3 direction, float intensity, Color tint,
              ShadowMap shadowMap) {
    static Shader sun = {0};
    static int dirLoc, intensityLoc, colorLoc, cascadeCountLoc, cascadeSizeLoc, cascadeLocs[8], cascadeMatLocs[8],
               cascadeDistsLocs[8];
    if (sun.id == 0) {
        sun = GetShader(SHADER_SUN);
        dirLoc = GetShaderLocation(sun, "direction");
        intensityLoc = GetShaderLocation(sun, "intensity");
        colorLoc = GetShaderLocation(sun, "color");
        cascadeCountLoc = GetShaderLocation(sun, "cascadeCount");
        cascadeSizeLoc = GetShaderLocation(sun, "cascadeSize");
        for (int i = 0; i < 8; i++) {
            cascadeLocs[i] = GetShaderLocation(sun, TextFormat("cascades[%i]", i));
            cascadeMatLocs[i] = GetShaderLocation(sun, TextFormat("cascadeMats[%i]", i));
            cascadeDistsLocs[i] = GetShaderLocation(sun, TextFormat("cascadeDists[%i]", i));
        }
    }

    direction = Vector3Normalize(direction);

    SetShaderValue(sun, dirLoc, &direction, SHADER_UNIFORM_VEC3);
    SetShaderValue(sun, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    Vector3 color = {(float) tint.r / 255.f, (float) tint.g / 255.f, (float) tint.b / 255.f};
    SetShaderValue(sun, colorLoc, &color, SHADER_UNIFORM_VEC3);

    SetShaderValue(sun, cascadeCountLoc, &shadowMap.cascades, SHADER_UNIFORM_INT);
    SetShaderValue(sun, cascadeSizeLoc, &shadowMap.size, SHADER_UNIFORM_INT);

    for (int i = 0; i < shadowMap.cascades; i++) {
        SetShaderValueMatrix(sun, cascadeMatLocs[i], shadowMap.projections[i]);
        SetShaderValue(sun, cascadeDistsLocs[i], &shadowMap.dists[i], SHADER_UNIFORM_FLOAT);
    }

    if (shadowMap.ids)
        RunLightShaderEx(presenter, camera, sun, 8, shadowMap.ids, cascadeLocs);
    else {
        const unsigned int dummyIDs[] = {0, 0, 0, 0, 0, 0, 0, 0};
        RunLightShaderEx(presenter, camera, sun, 8, dummyIDs, cascadeLocs);;
    }
}

const char rl3d_brdf_lut[] = {
#embed "assets/brdf.png"
    ,
    '\0'
};

void LightIBL(GBufferPresenter presenter, Camera camera, TextureCubemap radiance, TextureCubemap irradiance) {
    static Shader ibl = {0};
    static Texture brdf;
    if (ibl.id == 0) {
        ibl = GetShader(SHADER_IBL);
        brdf = LoadTextureFromImage(LoadImageFromMemory(".png", rl3d_brdf_lut, sizeof(rl3d_brdf_lut)));
    }

    unsigned int ids[] = {radiance.id, irradiance.id};
    int locs[] = {ibl.locs[SHADER_LOC_MAP_PREFILTER], ibl.locs[SHADER_LOC_MAP_IRRADIANCE]};

    RunLightShaderPro(presenter, camera, ibl, 2, ids, locs, 1, &brdf.id, &ibl.locs[SHADER_LOC_MAP_BRDF]);
}
