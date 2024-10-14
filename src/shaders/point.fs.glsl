#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D depth;
uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform vec3 pos;
uniform float intensity;
uniform float radius;
uniform vec4 color;

const float CULL_NEAR = 0.01;
const float CULL_FAR = 1000.0;

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

void main() {
    finalColor = vec4(0, 0, 0, 0);
    float dist = texture(depth, fragTexCoord).r;
    if (dist == 1) discard;
    vec3 worldPos = WorldPosFromDepth(dist);

    float lightDistance = distance(pos, worldPos);
    if (lightDistance * lightDistance > radius) return;
    vec3 lightDir = normalize(pos - worldPos);

    vec4 albedo = texture(albedoMap, fragTexCoord);
    vec3 normal = texture(normalMap, fragTexCoord).rgb;
    vec3 viewDir = normalize(camPos - worldPos);

    float diff = max(dot(lightDir, normal), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specular = pow(max(dot(normal, halfwayDir), 0.0), 32);

    float att = 1.0 / (lightDistance * lightDistance);
    finalColor = intensity * att * (diff + specular) * color * albedo;
}
