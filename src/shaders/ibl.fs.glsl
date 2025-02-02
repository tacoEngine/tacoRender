#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D occlusionMap;
uniform sampler2D emissiveMap;
uniform sampler2D depth;
uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform samplerCube radianceMap;
uniform samplerCube irradianceMap;
uniform sampler2D brdfLUT;
uniform float radianceMaps;

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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    finalColor = vec4(0, 0, 0, 0);
    float dist = texture(depth, fragTexCoord).r;
    if (dist >= maxDist) discard;
    vec3 worldPos = WorldPosFromDepth(dist);

    vec4 albedo = pow(texture(albedoMap, fragTexCoord), vec4(vec3(2.2), 1));
    vec3 normal = texture(normalMap, fragTexCoord).rgb;
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;
    float occlusion = texture(occlusionMap, fragTexCoord).r;
    vec3 emission = texture(emissiveMap, fragTexCoord).rgb;

    vec3 viewDir = normalize(camPos - worldPos);
    float viewAngle = max(dot(normal, viewDir), 0.0);
    vec3 viewReflect = reflect(-viewDir, normal);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);

    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = FresnelSchlickRoughness(viewAngle, F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo.rgb;

    //// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    vec3 prefilteredColor = textureLod(radianceMap, viewReflect, roughness * radianceMaps).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(viewAngle, roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * occlusion;

    finalColor = vec4(ambient + emission, albedo.a);
}
