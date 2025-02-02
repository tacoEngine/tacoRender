#version 330 core

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;
in vec3 fragTangent;
in vec2 fragTexCoord2;

layout (location = 0) out vec4 albedo;
layout (location = 1) out vec3 normal;
layout (location = 2) out float metallic;
layout (location = 3) out float roughness;
layout (location = 4) out vec3 emission;
layout (location = 5) out float ao;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D heightMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D emissionMap;
uniform sampler2D occlusionMap;

vec3 GetNormalFromMap() {
    vec3 tangentNormal = normalize(texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0);

    vec3 Q1  = dFdx(fragPosition);
    vec3 Q2  = dFdy(fragPosition);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 tangent;
    // Just approximate the tangent if it's not been uploaded
    if (fragTangent == vec3(0))
        tangent = normalize(Q1*st2.t - Q2*st1.t);
    else
     tangent = normalize(fragTangent);
    vec3 normal = normalize(fragNormal);
    vec3 bitangent = -normalize(cross(normal, tangent));

    mat3 TBN = mat3(tangent, bitangent, normal);
    return normalize(TBN * tangentNormal);
}

void main() {
    albedo    = texture(albedoMap, fragTexCoord).rgba;
    if (albedo.a == 0) discard;
    normal    = GetNormalFromMap();
    metallic  = texture(metallicMap, fragTexCoord).r;
    roughness = texture(roughnessMap, fragTexCoord).r;
    emission  = texture(emissionMap, fragTexCoord).rgb;
    ao        = texture(occlusionMap, fragTexCoord).r;
}
