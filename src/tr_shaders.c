// tacoRender (c) Nikolas Wipper 2023-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tr_shaders.h"

#include <raylib.h>
#include <stddef.h>

static Shader gBufferShader = {0};
static Shader depthDisplayShader = {0};
static Shader addShader = {0};
static Shader blurGaussShader = {0};
static Shader blurBoxShader = {0};
static Shader skyboxShader = {0};
static Shader flipYShader = {0};
static Shader pointShader = {0};
static Shader sunShader = {0};
static Shader iblShader = {0};
static Shader gammaShader = {0};
static Shader toneMapReinhardShader = {0};
static Shader irradianceShader = {0};
static Shader prefilterShader = {0};
static Shader sssPointShader = {0};
static Shader ssaoShader = {0};
static Shader texToDepthShader = {0};
static Shader copyBackgroundShader = {0};

// @formatter:off
const char tr_gbuf_vs[] = {
#embed "shaders/gbuf.vs.glsl"
    , '\0'
};

const char tr_gbuf_fs[] = {
#embed "shaders/gbuf.fs.glsl"
    , '\0'
};

const char tr_flip_vs[] = {
#embed "shaders/flip.vs.glsl"
    , '\0'
};

const char tr_depth_display_fs[] = {
#embed "shaders/depth_display.fs.glsl"
    , '\0'
};

const char tr_add_fs[] = {
#embed "shaders/add.fs.glsl"
    , '\0'
};

const char tr_blur_gauss_fs[] = {
#embed "shaders/blur_gauss.fs.glsl"
    , '\0'
};

const char tr_blur_box_fs[] = {
#embed "shaders/blur_box.fs.glsl"
    , '\0'
};

const char tr_skybox_vs[] = {
#embed "shaders/skybox.vs.glsl"
    , '\0'
};

const char tr_skybox_fs[] = {
#embed "shaders/skybox.fs.glsl"
    , '\0'
};

const char tr_point_fs[] = {
#embed "shaders/point.fs.glsl"
    , '\0'
};

const char tr_sun_fs[] = {
#embed "shaders/sun.fs.glsl"
    , '\0'
};

const char tr_ibl_fs[] = {
#embed "shaders/ibl.fs.glsl"
    , '\0'
};

const char tr_gamma_fs[] = {
#embed "shaders/gamma.fs.glsl"
    , '\0'
};

const char tr_tone_map_reinhard[] = {
#embed "shaders/tone_map_reinhard.fs.glsl"
    , '\0'
};

const char tr_cubemap_vs[] = {
#embed "shaders/cubemap.vs.glsl"
    , '\0'
};

const char tr_irradiance_fs[] = {
#embed "shaders/irradiance.fs.glsl"
    , '\0'
};

const char tr_prefilter_fs[] = {
#embed "shaders/prefilter.fs.glsl"
    , '\0'
};

const char tr_ssao_fs[] = {
#embed "shaders/ssao.fs.glsl"
    , '\0'
};

const char tr_tex_to_depth_fs[] = {
#embed "shaders/tex_to_depth.fs.glsl"
    , '\0'
};

const char tr_copy_background_fs[] = {
#embed "shaders/copy_background.fs.glsl"
    , '\0'
};
// @formatter:on

void LoadShaders() {
    if (gBufferShader.id == 0) {
        gBufferShader = LoadShaderFromMemory(tr_gbuf_vs, tr_gbuf_fs);
        gBufferShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(gBufferShader, "albedoMap");
        gBufferShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(gBufferShader, "normalMap");
        gBufferShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(gBufferShader, "heightMap");
        gBufferShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(gBufferShader, "metallicMap");
        gBufferShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(gBufferShader, "roughnessMap");
        gBufferShader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(gBufferShader, "emissionMap");
        gBufferShader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(gBufferShader, "occlusionMap");

        depthDisplayShader = LoadShaderFromMemory(tr_flip_vs, tr_depth_display_fs);
        addShader = LoadShaderFromMemory(NULL, tr_add_fs);
        blurGaussShader = LoadShaderFromMemory(tr_flip_vs, tr_blur_gauss_fs);
        blurBoxShader = LoadShaderFromMemory(tr_flip_vs, tr_blur_box_fs);
        skyboxShader = LoadShaderFromMemory(tr_skybox_vs, tr_skybox_fs);

        skyboxShader.locs[SHADER_LOC_MAP_CUBEMAP] = GetShaderLocation(skyboxShader, "skybox");

        flipYShader = LoadShaderFromMemory(tr_flip_vs, NULL);

        pointShader = LoadShaderFromMemory(NULL, tr_point_fs);
        pointShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(pointShader, "albedoMap");
        pointShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(pointShader, "normalMap");
        pointShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(pointShader, "metallicMap");
        pointShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(pointShader, "roughnessMap");
        pointShader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(pointShader, "emissiveMap");
        pointShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(pointShader, "depth");
        pointShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(pointShader, "camPos");

        sunShader = LoadShaderFromMemory(NULL, tr_sun_fs);
        sunShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(sunShader, "albedoMap");
        sunShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(sunShader, "normalMap");
        sunShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(sunShader, "metallicMap");
        sunShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(sunShader, "roughnessMap");
        sunShader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(sunShader, "emissiveMap");
        sunShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(sunShader, "depth");
        sunShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(sunShader, "camPos");

        iblShader = LoadShaderFromMemory(NULL, tr_ibl_fs);
        iblShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(iblShader, "albedoMap");
        iblShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(iblShader, "normalMap");
        iblShader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(iblShader, "metallicMap");
        iblShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(iblShader, "roughnessMap");
        iblShader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(iblShader, "occlusionMap");
        iblShader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(iblShader, "emissiveMap");
        iblShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(iblShader, "depth");
        iblShader.locs[SHADER_LOC_MAP_PREFILTER] = GetShaderLocation(iblShader, "radianceMap");
        iblShader.locs[SHADER_LOC_MAP_IRRADIANCE] = GetShaderLocation(iblShader, "irradianceMap");
        iblShader.locs[SHADER_LOC_MAP_BRDF] = GetShaderLocation(iblShader, "brdfLUT");
        iblShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(iblShader, "camPos");

        gammaShader = LoadShaderFromMemory(NULL, tr_gamma_fs);

        toneMapReinhardShader = LoadShaderFromMemory(NULL, tr_tone_map_reinhard);

        irradianceShader = LoadShaderFromMemory(tr_cubemap_vs, tr_irradiance_fs);

        prefilterShader = LoadShaderFromMemory(tr_cubemap_vs, tr_prefilter_fs);

        ssaoShader = LoadShaderFromMemory(tr_flip_vs, tr_ssao_fs);
        ssaoShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(ssaoShader, "normalMap");
        ssaoShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(ssaoShader, "depth");

        texToDepthShader = LoadShaderFromMemory(tr_flip_vs, tr_tex_to_depth_fs);

        copyBackgroundShader = LoadShaderFromMemory(NULL, tr_copy_background_fs);
        copyBackgroundShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(copyBackgroundShader, "albedoMap");
        copyBackgroundShader.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(copyBackgroundShader, "depth");
    }
}

void UnloadShaders() {
    UnloadShader(gBufferShader);
    UnloadShader(depthDisplayShader);
    UnloadShader(addShader);
    UnloadShader(blurGaussShader);
    UnloadShader(skyboxShader);
    UnloadShader(flipYShader);
    UnloadShader(pointShader);
    UnloadShader(sunShader);
    UnloadShader(gammaShader);
    UnloadShader(toneMapReinhardShader);
    UnloadShader(ssaoShader);
    UnloadShader(copyBackgroundShader);
}

Shader GetShader(EmbeddedShader shade) {
    switch (shade) {
    case SHADER_GBUF:
        return gBufferShader;
    case SHADER_DEPTH_DISPLAY:
        return depthDisplayShader;
    case SHADER_ADD:
        return addShader;
    case SHADER_BLUR_GAUSS:
        return blurGaussShader;
    case SHADER_BLUR_BOX:
        return blurBoxShader;
    case SHADER_SKYBOX:
        return skyboxShader;
    case SHADER_FLIP_Y:
        return flipYShader;
    case SHADER_POINT:
        return pointShader;
    case SHADER_SUN:
        return sunShader;
    case SHADER_IBL:
        return iblShader;
    case SHADER_GAMMA:
        return gammaShader;
    case SHADER_TONE_MAP_REINHARD:
        return toneMapReinhardShader;
    case SHADER_IRRADIANCE:
        return irradianceShader;
    case SHADER_PREFILTER:
        return prefilterShader;
    case SHADER_SSAO:
        return ssaoShader;
    case SHADER_TEX_TO_DEPTH:
        return texToDepthShader;
    case SHADER_COPY_BACKGROUND:
        return copyBackgroundShader;
    }
    return LoadMaterialDefault().shader;
}
