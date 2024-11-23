#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D depth;
uniform sampler2D cascades[8];
uniform mat4 cascadeMats[8];
uniform float cascadeDists[8];
uniform int cascadeCount;

uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform vec3 direction;
uniform float intensity;
uniform vec3 color;

const float CULL_NEAR = 0.01;
const float CULL_FAR = 1000.0;

const float PI = 3.14159265359;

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

float sampleCascade(int cascadeIndex, vec2 coords) {
    switch (cascadeIndex) {
        case 0:
            return texture(cascades[0], coords).x;
        case 1:
            return texture(cascades[1], coords).x;
        case 2:
            return texture(cascades[2], coords).x;
        case 3:
            return texture(cascades[3], coords).x;
        case 4:
            return texture(cascades[4], coords).x;
        case 5:
            return texture(cascades[5], coords).x;
        case 6:
            return texture(cascades[6], coords).x;
        case 7:
            return texture(cascades[7], coords).x;
    }
    return 0;
}

float CalcShadowFactor(int cascadeIndex, vec4 lightSpacePos, float lightAngle)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = 0.5 * projCoords + 0.5;

    if (projCoords.z > 1.0)
        return 1.0;

    float depth = sampleCascade(cascadeIndex, projCoords.xy);

    if (depth - 0.00001 > projCoords.z)
        return 1.0;
    else
        return 0.0;
}

void main() {
    finalColor = vec4(0, 0, 0, 0);
    float dist = texture(depth, fragTexCoord).r;
    if (dist == 1) discard;
    float linearDist = LinearizeDepth(dist);
    vec3 worldPos = WorldPosFromDepth(dist);

    vec4 albedo = texture(albedoMap, fragTexCoord);
    vec3 normal = texture(normalMap, fragTexCoord).rgb;
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;

    vec3 lightDir = normalize(-direction);
    vec3 viewDir = normalize(camPos - worldPos);

    float lightAngle = max(dot(lightDir, normal), 0.0);

    float shadowFactor = 0.0;
    vec3 cascadeColor = vec3(0);

    for (int i = 0; i < cascadeCount; i++) {
        if (linearDist <= cascadeDists[i]) {
            vec4 lightSpacePos = cascadeMats[i] * vec4(worldPos, 1);
            shadowFactor = CalcShadowFactor(i, lightSpacePos, lightAngle);
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

    finalColor = vec4(outColor * shadowFactor, albedo.a);
}
