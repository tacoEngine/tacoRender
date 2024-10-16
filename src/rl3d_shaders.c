// rl3d (c) Nikolas Wipper 2023

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "rl3d_shaders.h"

#include <raylib.h>
#include <stddef.h>

static Shader gBufferShader = {0};
static Shader depthDisplayShader = {0};
static Shader addShader = {0};
static Shader bloomShaderHorizontal = {0};
static Shader bloomShaderVertical = {0};
static Shader skyboxShader = {0};
static Shader flipYShader = {0};
static Shader pointShader = {0};
static Shader sunShader = {0};
static Shader gammaShader = {0};
static Shader toneMapReinhardShader = {0};

const char rl3d_gbuf_vs[] = {
#embed "shaders/gbuf.vs.glsl"
    ,'\0'
};

const char rl3d_gbuf_fs[] = {
#embed "shaders/gbuf.fs.glsl"
    ,'\0'
};

const char rl3d_flip_vs[] = {
#embed "shaders/flip.vs.glsl"
    ,'\0'
};

const char rl3d_depth_display_fs[] = {
#embed "shaders/depth_display.fs.glsl"
    ,'\0'
};

const char rl3d_add_fs[] = {
#embed "shaders/add.fs.glsl"
    ,'\0'
};

const char rl3d_blur_hor_fs[] = {
#embed "shaders/blur_hor.fs.glsl"
    ,'\0'
};

const char rl3d_blur_vert_fs[] = {
#embed "shaders/blur_vert.fs.glsl"
    ,'\0'
};

const char rl3d_skybox_vs[] = {
#embed "shaders/skybox.vs.glsl"
    ,'\0'
};

const char rl3d_skybox_fs[] = {
#embed "shaders/skybox.fs.glsl"
    ,'\0'
};

const char rl3d_point_fs[] = {
#embed "shaders/point.fs.glsl"
    ,'\0'
};

const char rl3d_sun_fs[] = {
#embed "shaders/sun.fs.glsl"
    ,'\0'
};

const char rl3d_gamma_fs[] = {
#embed "shaders/gamma.fs.glsl"
    ,'\0'
};

const char rl3d_tone_map_reinhard[] = {
#embed "shaders/tone_map_reinhard.fs.glsl"
    ,'\0'
};

void LoadShaders() {
    if (gBufferShader.id == 0) {
        gBufferShader = LoadShaderFromMemory(rl3d_gbuf_vs, rl3d_gbuf_fs);
        gBufferShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(gBufferShader, "albedoMap");
        gBufferShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(gBufferShader, "normalMap");
        gBufferShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(gBufferShader, "heightMap");
        gBufferShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(gBufferShader, "metallicMap");
        gBufferShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(gBufferShader, "roughnessMap");
        gBufferShader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(gBufferShader, "emissionMap");
        gBufferShader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(gBufferShader, "occlusionMap");

        depthDisplayShader = LoadShaderFromMemory(NULL, rl3d_depth_display_fs);
        addShader = LoadShaderFromMemory(NULL, rl3d_add_fs);
        bloomShaderHorizontal = LoadShaderFromMemory(NULL, rl3d_blur_hor_fs);
        bloomShaderVertical = LoadShaderFromMemory(NULL, rl3d_blur_vert_fs);
        skyboxShader = LoadShaderFromMemory(rl3d_skybox_vs, rl3d_skybox_fs);

        skyboxShader.locs[SHADER_LOC_MAP_CUBEMAP] = GetShaderLocation(skyboxShader, "skybox");

        flipYShader = LoadShaderFromMemory(rl3d_flip_vs, NULL);

        pointShader = LoadShaderFromMemory(NULL, rl3d_point_fs);
        pointShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(pointShader, "albedoMap");
        pointShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(pointShader, "normalMap");
        pointShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(pointShader, "metallicMap");
        pointShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(pointShader, "roughnessMap");
        pointShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(pointShader, "depth");
        pointShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(pointShader, "camPos");

        sunShader = LoadShaderFromMemory(NULL, rl3d_sun_fs);
        sunShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(sunShader, "albedoMap");
        sunShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(sunShader, "normalMap");
        sunShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(sunShader, "metallicMap");
        sunShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(sunShader, "roughnessMap");
        sunShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(sunShader, "depth");
        sunShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(sunShader, "camPos");

        gammaShader = LoadShaderFromMemory(NULL, rl3d_gamma_fs);

        toneMapReinhardShader = LoadShaderFromMemory(NULL, rl3d_tone_map_reinhard);
    }
}

void UnloadShaders() {
    UnloadShader(gBufferShader);
    UnloadShader(depthDisplayShader);
    UnloadShader(addShader);
    UnloadShader(bloomShaderHorizontal);
    UnloadShader(bloomShaderVertical);
    UnloadShader(skyboxShader);
    UnloadShader(flipYShader);
    UnloadShader(pointShader);
    UnloadShader(sunShader);
    UnloadShader(gammaShader);
    UnloadShader(toneMapReinhardShader);
}

Shader GetShader(EmbeddedShader shade) {
    switch (shade) {
        case SHADER_GBUF:
            return gBufferShader;
        case SHADER_DEPTH_DISPLAY:
            return depthDisplayShader;
        case SHADER_ADD:
            return addShader;
        case SHADER_BLUR_HOR:
            return bloomShaderHorizontal;
        case SHADER_BLUR_VERT:
            return bloomShaderVertical;
        case SHADER_SKYBOX:
            return skyboxShader;
        case SHADER_FLIP_Y:
            return flipYShader;
        case SHADER_POINT:
            return pointShader;
        case SHADER_SUN:
            return sunShader;
        case SHADER_GAMMA:
            return gammaShader;
        case SHADER_TONE_MAP_REINHARD:
            return toneMapReinhardShader;
    }
    return LoadMaterialDefault().shader;
}
