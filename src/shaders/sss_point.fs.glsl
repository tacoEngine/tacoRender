#version 330 core

in vec2 fragTexCoord;

out float finalColor;

uniform sampler2D depth;
uniform vec3 position;
uniform int offset;
uniform int maxSteps;
uniform mat4 viewMat;
uniform mat4 projMat;

const float CULL_NEAR = 0.01;
const float CULL_FAR = 1000.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * CULL_NEAR * CULL_FAR) / (CULL_FAR + CULL_NEAR - z * (CULL_FAR - CULL_NEAR));
}

float DelinearizeDepth(float depth) {
    float z = ((2.0 * CULL_NEAR * CULL_FAR) / depth) - (CULL_FAR + CULL_NEAR);
    return (z / (CULL_NEAR - CULL_FAR) + 1.0) * 0.5; // convert back to depth
}

vec3 ScreenSpaceFromWorld(vec3 worldPos) {
    // Transform world position to clip space
    vec4 clipSpacePos = projMat * viewMat * vec4(worldPos, 1.0);

    // Perform perspective division to get normalized device coordinates (NDC)
    vec3 ndc = clipSpacePos.xyz / clipSpacePos.w;

    // Convert NDC to screen space (range [0, 1])
    vec3 screenSpacePos = ndc.xyz * 0.5 + 0.5;

    // Return screen space X, Y, and depth (Z)
    return screenSpacePos;
}

float DepthFromPoint(vec2 position, vec2 lightPosition, float lightDepth) {
    float sam = LinearizeDepth(texture(depth, position).r);

    float lightDist = distance(position, lightPosition);
    float originDist = distance(position, fragTexCoord);

    float slope = (sam - lightDepth) / lightDist;

    return sam + slope * originDist;
}

void main() {
    vec3 lightPositionDepth = ScreenSpaceFromWorld(position);
    float linearLightDepth = LinearizeDepth(lightPositionDepth.z);

    vec2 toLight = lightPositionDepth.xy - fragTexCoord;
    vec2 stepSize = toLight / float(maxSteps);

    vec2 positions[4] = vec2[4](
        fragTexCoord,
        fragTexCoord + stepSize * float(offset),
        fragTexCoord + stepSize * float(offset * 2),
        fragTexCoord + stepSize * float(offset * 3)
    );

    float depth0 = DepthFromPoint(positions[0], lightPositionDepth.xy, linearLightDepth);
    float depth1 = DepthFromPoint(positions[1], lightPositionDepth.xy, linearLightDepth);
    float depth2 = DepthFromPoint(positions[2], lightPositionDepth.xy, linearLightDepth);
    float depth3 = DepthFromPoint(positions[3], lightPositionDepth.xy, linearLightDepth);

    float minDepth = min(min(min(depth0, depth1), depth2), depth3);

    finalColor = clamp(DelinearizeDepth(minDepth), 0.0, 1.0);
}
