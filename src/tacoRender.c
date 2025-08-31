// tacoRender (c) Nikolas Wipper 2023-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tacoRender.h"

#include "tr_shaders.h"

#include <rlgl.h>
#include <stddef.h>
#include <external/glad.h>

void Init3D() {
    LoadShaders();
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
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

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer
    target.width = width;
    target.height = height;

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.albedo = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        target.normal = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R16G16B16);
        target.metallic = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
        target.roughness = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
        target.emission = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        target.ao = LoadEmptyTexture(width, height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

        SetTextureFilter(target.emission, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(target.ao, TEXTURE_WRAP_CLAMP);

        // Create depth texture
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        rlActiveDrawBuffers(6);

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.albedo.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.normal.id, RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.metallic.id, RL_ATTACHMENT_COLOR_CHANNEL2, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.roughness.id, RL_ATTACHMENT_COLOR_CHANNEL3, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.emission.id, RL_ATTACHMENT_COLOR_CHANNEL4, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.ao.id, RL_ATTACHMENT_COLOR_CHANNEL5, RL_ATTACHMENT_TEXTURE2D, 0);
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

RenderTexture LoadCustomRenderTexture(int width, int height, int format, bool depthless, bool useRenderBuffer) {
    RenderTexture2D target = {0};

    target.id = rlLoadFramebuffer();

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.texture.id = rlLoadTexture(NULL, width, height, format, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = format;
        target.texture.mipmaps = 1;

        if (!depthless) {
            target.depth.id = rlLoadTextureDepth(width, height, useRenderBuffer);
            target.depth.width = width;
            target.depth.height = height;
            target.depth.format = 19;
            target.depth.mipmaps = 1;
            rlFramebufferAttach(target.id,
                                target.depth.id,
                                RL_ATTACHMENT_DEPTH,
                                useRenderBuffer ? RL_ATTACHMENT_RENDERBUFFER : RL_ATTACHMENT_TEXTURE2D,
                                0);
        }

        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);

        if (rlFramebufferComplete(target.id))
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    } else
        TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

GBufferPresenter LoadPresenter(GBuffers buffers) {
    GBufferPresenter presenter;
    // Todo: Make back buffer depthless
    presenter.back[0] = LoadCustomRenderTexture(buffers.width,
                                                buffers.height,
                                                PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,
                                                false,
                                                true);
    presenter.back[1] = LoadCustomRenderTexture(buffers.width,
                                                buffers.height,
                                                PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,
                                                false,
                                                true);

    SetTextureWrap(presenter.back[0].texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(presenter.back[1].texture, TEXTURE_WRAP_CLAMP);

    presenter.source = buffers;

    presenter.occlusion.id = rlLoadFramebuffer();
    if (presenter.occlusion.id > 0) {
        rlEnableFramebuffer(presenter.occlusion.id);

        presenter.occlusion.texture.width = buffers.width;
        presenter.occlusion.texture.height = buffers.height;

        presenter.occlusion.depth.id = rlLoadTextureDepth(buffers.width, buffers.height, true);
        presenter.occlusion.depth.width = buffers.width;
        presenter.occlusion.depth.height = buffers.height;
        presenter.occlusion.depth.format = 19;
        presenter.occlusion.depth.mipmaps = 1;

        rlFramebufferAttach(presenter.occlusion.id,
                            buffers.ao.id,
                            RL_ATTACHMENT_COLOR_CHANNEL0,
                            RL_ATTACHMENT_TEXTURE2D,
                            0);
        rlFramebufferAttach(presenter.occlusion.id,
                            presenter.occlusion.depth.id,
                            RL_ATTACHMENT_DEPTH,
                            RL_ATTACHMENT_RENDERBUFFER,
                            0);

        if (rlFramebufferComplete(presenter.occlusion.id))
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }

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

    DrawTexture(presenter.back[0].texture, 0, 0, WHITE);

    EndDrawing();
}

void AddBackBuffer(GBufferPresenter presenter) {
    rlEnableColorBlend();

    BeginTextureMode(presenter.back[0]);

    BeginBlendMode(BLEND_ADD_COLORS);

    BeginShaderMode(GetShader(SHADER_FLIP_Y));

    DrawTexture(presenter.back[0].texture, 0, 0, WHITE);

    EndShaderMode();

    EndBlendMode();

    EndTextureMode();

    rlDisableColorBlend();
}

#ifndef NO_CONVENIENCE

void DrawDepth(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    BeginShaderMode(GetShader(SHADER_DEPTH_DISPLAY));
    DrawTextureEx(texture, position, rotation, scale, tint);
    EndShaderMode();
}

#endif
