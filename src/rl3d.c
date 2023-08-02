// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "rl3d.h"

#include "rl3d_shaders.h"

#include <rlgl.h>
#include <stddef.h>

void Init3D() {
    LoadShaders();
}

void Uninit3D() {
    UnloadShaders();
}

Texture LoadEmptyTexture(int width, int height, PixelFormat format) {
    Texture texture;
    texture.id = rlLoadTexture(NULL, width, height, format, 1);
    texture.width = width;
    texture.height = height;
    texture.format = format;
    texture.mipmaps = 1;
    return texture;
}

GBuffers LoadGBuffers(int width, int height) {
    GBuffers target = {0};

    target.id = rlLoadFramebuffer(width, height);   // Load an empty framebuffer

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.albedo = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        target.normal = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R32G32B32);
        target.height = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        target.metallic = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        target.roughness = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        target.emission = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        target.ao = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

        SetTextureFilter(target.emission, TEXTURE_FILTER_BILINEAR);

        // Create depth texture
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        rlActiveDrawBuffers(7);

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.albedo.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.normal.id, RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.height.id, RL_ATTACHMENT_COLOR_CHANNEL2, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.metallic.id, RL_ATTACHMENT_COLOR_CHANNEL3, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.roughness.id, RL_ATTACHMENT_COLOR_CHANNEL4, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.emission.id, RL_ATTACHMENT_COLOR_CHANNEL5, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.ao.id, RL_ATTACHMENT_COLOR_CHANNEL6, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id))
            TraceLog(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);
        else
            TraceLog(LOG_WARNING, "FBO: [ID %i] Framebuffer object is incomplete", target.id);

        rlDisableFramebuffer();
    } else
        TraceLog(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

RenderTexture LoadHDRRenderTexture(int width, int height) {
    RenderTexture2D target = {0};

    target.id = rlLoadFramebuffer(width, height);

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.texture.id = rlLoadTexture(NULL, width, height, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        target.texture.mipmaps = 1;

        target.depth.id = rlLoadTextureDepth(width, height, true);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;
        target.depth.mipmaps = 1;

        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

        if (rlFramebufferComplete(target.id))
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    } else
        TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

GBufferPresenter LoadPresenter(GBuffers buffers) {
    GBufferPresenter presenter;
    // Todo: Make the presenter target use an HDR color texture
    presenter.target = LoadHDRRenderTexture(buffers.albedo.width, buffers.albedo.height);
    // note: back buffer doesn't need to be hdr, but
    // Todo: Make back buffer depthless
    presenter.back[0] = LoadHDRRenderTexture(buffers.albedo.width, buffers.albedo.height);
    presenter.back[1] = LoadHDRRenderTexture(buffers.albedo.width, buffers.albedo.height);
    presenter.source = buffers;

    SetTextureFilter(presenter.back[0].texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(presenter.back[1].texture, TEXTURE_FILTER_BILINEAR);

    return presenter;
}

Shader GetGBufferShader() {
    return GetShader(SHADER_GBUF);
}

void BeginGBufferMode(GBuffers buffers) {
    RenderTexture dummy = (RenderTexture) {.id = buffers.id, .texture = buffers.albedo, .depth = buffers.depth};
    BeginTextureMode(dummy);
    rlDisableColorBlend();
}

void EndGBufferMode() {
    EndTextureMode();
}

void ClearPresenter(GBufferPresenter presenter) {
    BeginTextureMode(presenter.target);
    rlClearScreenBuffers();
    EndTextureMode();

    BeginTextureMode(presenter.back[0]);
    rlClearScreenBuffers();
    EndTextureMode();

    BeginTextureMode(presenter.back[1]);
    rlClearScreenBuffers();
    EndTextureMode();
}

void Present(GBufferPresenter presenter) {
    BeginDrawing();

    ClearBackground(BLANK);

    DrawTexture(presenter.target.texture, 0, 0, WHITE);

    EndDrawing();
}

void AddBackBuffer(GBufferPresenter presenter) {
    rlEnableColorBlend();

    BeginTextureMode(presenter.target);

    BeginBlendMode(BLEND_ADD_COLORS);

    BeginShaderMode(GetShader(SHADER_FLIP_Y));

    DrawTexture(presenter.back[0].texture, 0, 0, WHITE);

    EndShaderMode();

    EndBlendMode();

    EndTextureMode();

    rlDisableColorBlend();
}

void SetBackbufferFilter(GBufferPresenter presenter, int filter) {
    SetTextureFilter(presenter.back[0].texture, filter);
    SetTextureFilter(presenter.back[1].texture, filter);
}

#ifndef NO_CONVENIENCE

void DrawDepth(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    BeginShaderMode(GetShader(SHADER_DEPTH_DISPLAY));
    DrawTextureEx(texture, position, rotation, scale, tint);
    EndShaderMode();
}

#endif
