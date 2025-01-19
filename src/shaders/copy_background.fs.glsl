#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D albedoMap;
uniform sampler2D depth;

// Biggest number less than 1 (one) representable with IEEE754 (or really close to it)
const float maxDist = 0.999999940395355224609375;

void main() {
    float dist = texture(depth, fragTexCoord).r;
    vec4 albedo = pow(texture(albedoMap, fragTexCoord), vec4(vec3(2.2), 1));
    if (dist >= maxDist) {
        finalColor = albedo;
    } else {
        discard;
    }
}
