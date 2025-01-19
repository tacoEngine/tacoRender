// tacoRender (c) Nikolas Wipper 2024-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tr_generators.h"

#include <rlgl.h>
#include <raymath.h>
#include <external/glad.h>

#include "tr_shaders.h"

static TextureCubemap GenTextureCubemap(Shader shader, TextureCubemap panorama, int size, int mipmapCount);

int ilog2(int n) {
    if (n <= 0)
        return -1; // Return -1 for undefined log2 on non-positive integers

    int result = 0;
    while (n >>= 1) // Right shift n until it becomes 0
        result++;

    return result;
}

TextureCubemap IrradianceCubemap(TextureCubemap cubemap) {
    static Shader irradiance = {0};
    if (irradiance.id == 0) {
        irradiance = GetShader(SHADER_IRRADIANCE);
    }

    return GenTextureCubemap(irradiance, cubemap, 32, 1);
}

TextureCubemap PrefilterCubemap(TextureCubemap cubemap) {
    static Shader irradiance = {0};
    if (irradiance.id == 0) {
        irradiance = GetShader(SHADER_PREFILTER);
    }

    return GenTextureCubemap(irradiance, cubemap, cubemap.width, ilog2(cubemap.width) + 1);
}

// Generate cubemap texture from HDR texture
static TextureCubemap GenTextureCubemap(Shader shader, TextureCubemap panorama, int size, int mipmapCount) {
    TextureCubemap cubemap = {0};

    int mipmapLevelLoc = GetShaderLocation(shader, "mipmapLevel");
    int maxMipmapLevelLoc = GetShaderLocation(shader, "maxMipmapLevel");
    SetShaderValue(shader, GetShaderLocation(shader, "cubemap"), (int[1]) {0}, SHADER_UNIFORM_INT);

    rlDisableBackfaceCulling(); // Disable backface culling to render inside the cube

    unsigned int fbo = rlLoadFramebuffer();
    unsigned int rbo = rlLoadTextureDepth(size, size, true);

    cubemap.id = rlLoadTextureCubemap(0, size, panorama.format, mipmapCount);

    rlCubemapParameters(cubemap.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_MIP_LINEAR);

    rlEnableShader(shader.id);

    // Define projection matrix and send it to shader
    Matrix matFboProjection = MatrixPerspective(90.0 * DEG2RAD, 1.0, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matFboProjection);

    // Define view matrix for every side of the cubemap
    Matrix fboViews[6] = {
        MatrixLookAt((Vector3) {0.0f, 0.0f, 0.0f}, (Vector3) {1.0f, 0.0f, 0.0f}, (Vector3) {0.0f, -1.0f, 0.0f}),
        MatrixLookAt((Vector3) {0.0f, 0.0f, 0.0f}, (Vector3) {-1.0f, 0.0f, 0.0f}, (Vector3) {0.0f, -1.0f, 0.0f}),
        MatrixLookAt((Vector3) {0.0f, 0.0f, 0.0f}, (Vector3) {0.0f, 1.0f, 0.0f}, (Vector3) {0.0f, 0.0f, 1.0f}),
        MatrixLookAt((Vector3) {0.0f, 0.0f, 0.0f}, (Vector3) {0.0f, -1.0f, 0.0f}, (Vector3) {0.0f, 0.0f, -1.0f}),
        MatrixLookAt((Vector3) {0.0f, 0.0f, 0.0f}, (Vector3) {0.0f, 0.0f, 1.0f}, (Vector3) {0.0f, -1.0f, 0.0f}),
        MatrixLookAt((Vector3) {0.0f, 0.0f, 0.0f}, (Vector3) {0.0f, 0.0f, -1.0f}, (Vector3) {0.0f, -1.0f, 0.0f})
    };

    // Activate and enable texture for drawing to cubemap faces
    rlActiveTextureSlot(0);
    rlEnableTextureCubemap(panorama.id);

    if (maxMipmapLevelLoc != -1)
        SetShaderValue(shader, maxMipmapLevelLoc, &mipmapCount, SHADER_UNIFORM_INT);

    int mipmapSize = size;

    for (int mip = 0; mip < mipmapCount; ++mip) {
        // resize framebuffer according to mip-level size.
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipmapSize, mipmapSize);
        rlViewport(0, 0, mipmapSize, mipmapSize);

        if (mipmapLevelLoc >= 0)
            SetShaderValue(shader, mipmapLevelLoc, &mip, SHADER_UNIFORM_INT);

        for (int i = 0; i < 6; ++i) {
            // Set the view matrix for the current cube face
            rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], fboViews[i]);

            // Select the current cubemap face attachment for the fbo
            // WARNING: This function by default enables->attach->disables fbo!!!
            rlFramebufferAttach(fbo,
                                cubemap.id,
                                RL_ATTACHMENT_COLOR_CHANNEL0,
                                RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i,
                                mip);
            rlEnableFramebuffer(fbo);

            // Load and draw a cube, it uses the current enabled texture
            rlClearScreenBuffers();
            rlLoadDrawCube();
        }
        mipmapSize /= 2;
    }

    rlDisableShader(); // Unbind shader
    rlDisableTextureCubemap(); // Unbind texture
    rlDisableFramebuffer(); // Unbind framebuffer
    rlUnloadFramebuffer(fbo); // Unload framebuffer (and automatically attached depth texture/renderbuffer)

    // Reset viewport dimensions to default
    rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());
    rlEnableBackfaceCulling();

    cubemap.width = size;
    cubemap.height = size;
    cubemap.mipmaps = mipmapCount;
    cubemap.format = panorama.format;

    return cubemap;
}
