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

#define LINEARIZE_DEPTH "const float CULL_NEAR = 0.01;" \
"const float CULL_FAR = 1000.0;\n" \
"float LinearizeDepth(float depth) {\n" \
"float z = depth * 2.0 - 1.0; // back to NDC\n" \
"return (2.0 * CULL_NEAR * CULL_FAR) / (CULL_FAR + CULL_NEAR - z * (CULL_FAR - CULL_NEAR));\n" \
"}\n"

#define WORLD_POS_FROM_DEPTH "vec3 WorldPosFromDepth(float depth) {\n" \
"float z = depth * 2.0 - 1.0;\n" \
"vec4 clipSpacePosition = vec4(fragTexCoord * 2.0 - 1.0, z, 1.0);\n" \
"vec4 viewSpacePosition = invProj * clipSpacePosition;\n" \
"viewSpacePosition /= viewSpacePosition.w;\n" \
"vec4 worldSpacePosition = invView * viewSpacePosition;\n" \
"return worldSpacePosition.xyz;\n" \
"}\n"

const char *rl3d_gbuf_vs = "#version 330 core\n"
                           "in vec3 vertexPosition;\n"
                           "in vec2 vertexTexCoord;\n"
                           "in vec3 vertexNormal;\n"
                           "in vec4 vertexColor;\n"
                           "in vec3 vertexTangent;\n"
                           "in vec2 vertexTexCoord2;\n"
                           "out vec3 fragPosition;\n"
                           "out vec2 fragTexCoord;\n"
                           "out vec3 fragNormal;\n"
                           "out vec4 fragColor;\n"
                           "out vec3 fragTangent;\n"
                           "out vec2 fragTexCoord2;\n"
                           "uniform mat4 mvp;\n"
                           "uniform mat4 matModel;\n"
                           "uniform mat4 matNormal;\n"
                           "void main() {\n"
                           "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
                           "    fragTexCoord = vertexTexCoord;\n"
                           "    fragNormal = mat3(matNormal) * vertexNormal;\n"
                           "    fragColor = vertexColor;\n"
                           "    fragTangent = vertexTangent;\n"
                           "    fragTexCoord2 = vertexTexCoord2;\n"

                           "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
                           "}";

const char *rl3d_gbuf_fs = "#version 330 core\n"
                           "in vec3 fragPosition;\n"
                           "in vec2 fragTexCoord;\n"
                           "in vec3 fragNormal;\n"
                           "in vec4 fragColor;\n"
                           "in vec3 fragTangent;\n"
                           "in vec2 fragTexCoord2;\n"
                           "layout (location = 0) out vec4 albedo;\n"
                           "layout (location = 1) out vec3 normal;\n"
                           "layout (location = 2) out vec3 height;\n"
                           "layout (location = 3) out vec3 metallic;\n"
                           "layout (location = 4) out vec3 roughness;\n"
                           "layout (location = 5) out vec3 emission;\n"
                           "layout (location = 6) out float ao;\n"
                           "uniform sampler2D albedoMap;\n"
                           "uniform sampler2D normalMap;\n"
                           "uniform sampler2D heightMap;\n"
                           "uniform sampler2D metallicMap;\n"
                           "uniform sampler2D roughnessMap;\n"
                           "uniform sampler2D emissionMap;\n"
                           "uniform sampler2D occlusionMap;\n"

                           "vec3 GetNormalFromMap() {\n"
                           "    vec3 tangentNormal = normalize(texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0);\n"

                           "    vec3 Q1  = dFdx(fragPosition);\n"
                           "    vec3 Q2  = dFdy(fragPosition);\n"
                           "    vec2 st1 = dFdx(fragTexCoord);\n"
                           "    vec2 st2 = dFdy(fragTexCoord);\n"

                           "    vec3 N   = normalize(fragNormal);\n"
                           "    vec3 T   = normalize(Q1*st2.t - Q2*st1.t);\n"
                           "    vec3 B   = -normalize(cross(N, T));\n"
                           "    mat3 TBN = mat3(T, B, N);\n"

                           "    return normalize(TBN * tangentNormal);\n"
                           "}\n"

                           "void main() {\n"
                           "    albedo    = texture(albedoMap, fragTexCoord).rgba;\n"
                           "    normal    = GetNormalFromMap();\n"
                           "    height    = texture(heightMap, fragTexCoord).rgb;\n"
                           "    metallic  = texture(metallicMap, fragTexCoord).rgb;\n"
                           "    roughness = texture(roughnessMap, fragTexCoord).rgb;\n"
                           "    emission  = texture(emissionMap, fragTexCoord).rgb;\n"
                           "    ao        = texture(occlusionMap, fragTexCoord).r;\n"
                           "}";

const char *rl3d_flip_vs = "#version 330                       \n"
                           "in vec3 vertexPosition;            \n"
                           "in vec2 vertexTexCoord;            \n"
                           "in vec4 vertexColor;               \n"
                           "out vec2 fragTexCoord;             \n"
                           "out vec4 fragColor;                \n"
                           "uniform mat4 mvp;                  \n"
                           "void main() {                      \n"
                           "    fragTexCoord = vec2(vertexTexCoord.x, 1 - vertexTexCoord.y); \n"
                           "    fragColor = vertexColor;       \n"
                           "    gl_Position = mvp*vec4(vertexPosition, 1.0); \n"
                           "}                                  \n";

const char *rl3d_depth_display_fs = "#version 330 core\n"
                                    "in vec2 fragTexCoord;\n"
                                    "out vec4 finalColor;\n"
                                    "uniform sampler2D depth;\n"

                                    "float linearizeDepth(float d, float zNear, float zFar) {\n"
                                    "    float z_n = 2.0 * d - 1.0;\n"
                                    "    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));\n"
                                    "}\n"

                                    "void main() {\n"
                                    "    float depth = texture(depth, fragTexCoord).r;\n"
                                    "    finalColor = vec4(vec3(linearizeDepth(depth, 0.01, 2.0)), 1);\n"
                                    "}";

const char *rl3d_add_fs = "#version 330 core\n"
                          "in vec2 fragTexCoord;\n"
                          "out vec4 finalColor;\n"
                          "uniform sampler2D texture0;\n"
                          "uniform sampler2D texture1;\n"

                          "void main() {\n"
                          "    vec4 texel0 = texture(texture0, fragTexCoord);\n"
                          "    vec4 texel1 = texture(texture1, fragTexCoord);"
                          "    finalColor = texel0 + texel1;\n"
                          "}";

const char *rl3d_blur_hor_fs = "#version 330 core\n"
                               "in vec2 fragTexCoord;\n"
                               "out vec4 finalColor;\n"
                               "uniform sampler2D image;\n"
                               "uniform float radius;\n"

                               "uniform float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);\n"
                               "uniform float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);\n"

                               "void main() {"
                               "    vec2 tex_offset = 1.0 / textureSize(image, 0) * radius; // gets size of single texel\n"
                               "    vec4 result = texture(image, fragTexCoord) * weight[0]; // current fragment's contribution\n"
                               "    for(int i = 1; i < 3; ++i) {\n"
                               "        result += texture(image, fragTexCoord + vec2(offset[i] * tex_offset.x, 0.0)) * weight[i];\n"
                               "        result += texture(image, fragTexCoord - vec2(offset[i] * tex_offset.x, 0.0)) * weight[i];\n"
                               "    }\n"
                               "    finalColor = result;\n"
                               "}";

const char *rl3d_blur_vert_fs = "#version 330 core\n"
                                "in vec2 fragTexCoord;\n"
                                "out vec4 finalColor;\n"
                                "uniform sampler2D image;\n"
                                "uniform float radius;\n"

                                "uniform float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);\n"
                                "uniform float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);\n"

                                "void main() {"
                                "    vec2 tex_offset = 1.0 / textureSize(image, 0) * radius; // gets size of single texel\n"
                                "    vec4 result = texture(image, fragTexCoord) * weight[0]; // current fragment's contribution\n"
                                "    for(int i = 1; i < 3; ++i) {\n"
                                "        result += texture(image, fragTexCoord + vec2(0.0, offset[i] * tex_offset.y)) * weight[i];\n"
                                "        result += texture(image, fragTexCoord - vec2(0.0, offset[i] * tex_offset.y)) * weight[i];\n"
                                "    }\n"
                                "    finalColor = result;\n"
                                "}";

const char *rl3d_skybox_vs = "#version 330 core\n"
                             "in vec3 vertexPosition;\n"
                             "out vec3 fragTexCoord;\n"
                             "uniform mat4 matModel;\n"
                             "uniform mat4 matProjection;\n"
                             "uniform mat4 matView;\n"
                             ""
                             "void main() {\n"
                             "    mat4 rotView = mat4(mat3(matView));\n"
                             "    fragTexCoord = vertexPosition;\n"
                             "    gl_Position = (matProjection * rotView * vec4(vertexPosition, 1.0)).xyww;\n"
                             "}";

const char *rl3d_skybox_fs = "#version 330 core\n"
                             "in vec3 fragTexCoord;\n"
                             "out vec4 finalColor;\n"
                             "uniform samplerCube skybox;\n"
                             ""
                             "void main() {\n"
                             "    finalColor = texture(skybox, fragTexCoord + vec3(0, 0, 0));\n"
                             "}";

const char *rl3d_point_fs = "#version 330 core\n"
                            "in vec2 fragTexCoord;\n"
                            "out vec4 finalColor;\n"
                            "uniform sampler2D albedoMap;\n"
                            "uniform sampler2D normalMap;\n"
                            "uniform sampler2D depth;\n"
                            "uniform vec3 camPos;\n"
                            "uniform mat4 invView;\n"
                            "uniform mat4 invProj;\n"
                            "uniform vec3 pos;\n"
                            "uniform float intensity;\n"
                            "uniform float radius;\n"
                            "uniform vec4 color;\n"
                            LINEARIZE_DEPTH
                            WORLD_POS_FROM_DEPTH
                            "void main() {\n"
                            "    finalColor = vec4(0, 0, 0, 0);"

                            "    vec4 albedo = texture(albedoMap, fragTexCoord);\n"
                            "    vec3 normal = texture(normalMap, fragTexCoord).xyz;\n"
                            "    float dist = texture(depth, fragTexCoord).r;\n"
                            "    vec3 worldPos = WorldPosFromDepth(dist);\n"
                            "    vec3 viewDir = normalize(camPos - worldPos);\n"

                            "    float lightDistance = distance(pos, worldPos);\n"
                            "    if (lightDistance * lightDistance > radius) return;\n"
                            "    vec3 lightDir = normalize(pos - worldPos);\n"

                            "    float diff = max(dot(lightDir, normal), 0.0);\n"

                            "    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
                            "    float specular = pow(max(dot(normal, halfwayDir), 0.0), 32);\n"

                            "    float att = 1.0 / (lightDistance * lightDistance);\n"
                            "    finalColor = intensity * att * (diff + specular) * color * albedo;\n"
                            "}";

const char *rl3d_sun_fs = "#version 330 core\n"
                          "in vec2 fragTexCoord;\n"
                          "out vec4 finalColor;\n"
                          "uniform sampler2D albedoMap;\n"
                          "uniform sampler2D normalMap;\n"
                          "uniform sampler2D depth;\n"
                          "uniform vec3 camPos;\n"
                          "uniform mat4 invView;\n"
                          "uniform mat4 invProj;\n"
                          "uniform vec3 direction;\n"
                          "uniform float intensity;\n"
                          "uniform vec4 color;\n"
                          LINEARIZE_DEPTH
                          WORLD_POS_FROM_DEPTH
                          "void main() {\n"
                          "    finalColor = vec4(0, 0, 0, 0);"

                          "    vec4 albedo = texture(albedoMap, fragTexCoord);\n"
                          "    vec3 normal = texture(normalMap, fragTexCoord).xyz;\n"
                          "    float dist = texture(depth, fragTexCoord).r;\n"
                          "    vec3 worldPos = WorldPosFromDepth(dist);\n"
                          "    vec3 viewDir = normalize(camPos - worldPos);\n"

                          "    vec3 lightDir = -direction;\n"

                          "    float diff = max(dot(lightDir, normal), 0.0);\n"

                          "    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
                          "    float specular = pow(max(dot(normal, halfwayDir), 0.0), 32);\n"

                          "    finalColor = intensity * (diff + specular) * color * albedo;\n"
                          "}";

const char *rl3d_gamma = "#version 330 core\n"
                         "in vec2 fragTexCoord;\n"
                         "out vec4 finalColor;\n"
                         "uniform sampler2D source;\n"
                         "uniform float gamma = 2.2;\n"
                         "void main() {\n"
                         "    vec4 color = texture(source, fragTexCoord);\n"
                         "    vec3 corrected = pow(color.rgb, vec3(1.0 / gamma));"
                         "    finalColor = vec4(corrected, color.a);"
                         "}";

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
        pointShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(pointShader, "camPos");

        sunShader = LoadShaderFromMemory(NULL, rl3d_sun_fs);
        sunShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(sunShader, "albedoMap");
        sunShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(sunShader, "normalMap");
        sunShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(sunShader, "camPos");

        gammaShader = LoadShaderFromMemory(NULL, rl3d_gamma);
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
    }
    return LoadMaterialDefault().shader;
}
