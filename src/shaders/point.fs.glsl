#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D emissiveMap;
uniform sampler2D depth;
uniform sampler2D shadow;
uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform vec3 pos;
uniform float intensity;
uniform float radius;
uniform vec3 color;

const float CULL_NEAR = 0.05;
const float CULL_FAR = 4000.0;

const float PI = 3.14159265359;
// Biggest number less than 1 (one) representable with IEEE754 (or really close to it)
const float maxDist = 0.999999940395355224609375;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;// back to NDC
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
    float hDotN  = max(dot(H, normal), 0.0);
    float hDotN2 = hDotN*hDotN;

    float num   = a2;
    float denom = (hDotN2 * (a2 - 1.0) + 1.0);
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

float GeometrySmith(float vDotN, float lDotN, float roughness) {
    float ggx2  = GeometrySchlickGGX(vDotN, roughness);
    float ggx1  = GeometrySchlickGGX(lDotN, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float vDotH, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - vDotH, 0.0, 1.0), 5.0);
}

vec3 BurleyDiffuse(vec3 albedo, float lDotN, float vDotN, float vDotH, float roughness) {
    float F90 = 0.5 + 2 * roughness * vDotH;
    return albedo / PI * (1 + (F90 - 1) * pow(1 - lDotN, 5.0)) * (1 + (F90 - 1) * pow(1 - vDotN, 5.0));
}

void main() {
    finalColor = vec4(0, 0, 0, 0);
    float dist = texture(depth, fragTexCoord).r;
    if (dist >= maxDist) discard;
    vec3 worldPos = WorldPosFromDepth(dist);

    float lightDistance = distance(pos, worldPos);
    if (lightDistance * lightDistance > radius) discard;

    vec4 albedo = pow(texture(albedoMap, fragTexCoord), vec4(vec3(2.2), 1));
    vec3 normal = texture(normalMap, fragTexCoord).rgb;
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;
    vec3 emission = texture(emissiveMap, fragTexCoord).rgb;

    vec3 lightDir = normalize(pos - worldPos);
    vec3 viewDir = normalize(camPos - worldPos);

    float shadowFactor = 1;
    float shadowDepth = texture(shadow, fragTexCoord).r;

    if (LinearizeDepth(shadowDepth) < (LinearizeDepth(dist) - 0.01))
        shadowFactor = 0;

    vec3 halfwayDir = normalize(lightDir + viewDir);

    float lDotN = max(dot(lightDir, normal), 0.0);
    float vDotN = max(dot(viewDir, normal), 0.0);
    float vDotH = max(dot(viewDir, halfwayDir), 0.0);

    float att = pow(clamp(1 - pow(lightDistance/radius, 4), 0.0, 1.0), 2) / (lightDistance * lightDistance + 1);

    vec3 radiance = color * att * intensity;

    // cook-torrance brdf
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(vDotN, lDotN, roughness);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    vec3 F = FresnelSchlick(vDotH, F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * vDotN * lDotN + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 diffuse = BurleyDiffuse(albedo.rgb, lDotN, vDotN, vDotH, roughness);

    vec3 outColor = (kD * diffuse + specular) * radiance * lDotN;

    finalColor = vec4(outColor * shadowFactor + emission, albedo.a);
}
