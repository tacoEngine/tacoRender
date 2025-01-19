#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D emissiveMap;
uniform sampler2D depth;
uniform sampler2D cascades[8];
uniform mat4 cascadeMats[8];
uniform float cascadeDists[8];
uniform int cascadeCount;
uniform int cascadeSize;

uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform vec3 direction;
uniform float intensity;
uniform vec3 color;

const float CULL_NEAR = 0.01;
const float CULL_FAR = 1000.0;

const float PI = 3.14159265359;
// Biggest number less than 1 (one) representable with IEEE754 (or really close to it)
const float maxDist = 0.999999940395355224609375;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * CULL_NEAR * CULL_FAR) / (CULL_FAR + CULL_NEAR - z * (CULL_FAR - CULL_NEAR));
}

vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(fragTexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = invProj * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = invView * viewSpacePosition;
    return worldSpacePosition.xyz;
}

float DistributionGGX(vec3 normal, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(normal, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float SampleCascade(int cascadeIndex, vec3 coords) {
    switch (cascadeIndex) {
        case 0:
            return textureProj(cascades[0], coords).r;
        case 1:
            return textureProj(cascades[1], coords).r;
        case 2:
            return textureProj(cascades[2], coords).r;
        case 3:
            return textureProj(cascades[3], coords).r;
        case 4:
            return textureProj(cascades[4], coords).r;
        case 5:
            return textureProj(cascades[5], coords).r;
        case 6:
            return textureProj(cascades[6], coords).r;
        case 7:
            return textureProj(cascades[7], coords).r;
    }
    return 1.0;
}

float CalcShadowFactor(int cascadeIndex, vec4 lightSpacePos) {
    float d = (lightSpacePos.z / lightSpacePos.w) * 0.5 + 0.5;

    if (d > 1.0)
        return 1.0;

    float z = SampleCascade(cascadeIndex, lightSpacePos.xyw * 0.5 + 0.5);
    const float c = 160;

    float shadow = exp(-c * (d - z));

    return clamp(shadow, 0, 1);
}

void main() {
    finalColor = vec4(0, 0, 0, 0);
    float dist = texture(depth, fragTexCoord).r;
    if (dist >= maxDist) discard;
    float linearDist = LinearizeDepth(dist);
    vec3 worldPos = WorldPosFromDepth(dist);

    vec4 albedo = pow(texture(albedoMap, fragTexCoord), vec4(vec3(2.2), 1));
    vec3 normal = texture(normalMap, fragTexCoord).rgb;
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;
    vec3 emission = texture(emissiveMap, fragTexCoord).rgb;

    vec3 lightDir = normalize(-direction);
    vec3 viewDir = normalize(camPos - worldPos);

    float lightAngle = max(dot(lightDir, normal), 0.0);

    float shadowFactor = 1.0;

    for (int i = 0; i < cascadeCount; i++) {
        if (linearDist <= cascadeDists[i]) {
            vec4 lightSpacePos = cascadeMats[i] * vec4(worldPos, 1);
            shadowFactor = CalcShadowFactor(i, lightSpacePos);
            break;
        }
    }

    vec3 halfwayDir = normalize(lightDir + viewDir);

    vec3 radiance = color * intensity;

    // cook-torrance brdf
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    vec3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * lightAngle + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 outColor = (kD * albedo.rgb / PI + specular) * radiance * lightAngle;

    finalColor = vec4(outColor * shadowFactor + emission, albedo.a);
}
