#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D image;
uniform float radius;
uniform float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
uniform float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0) * radius; // gets size of single texel
    vec4 result = texture(image, fragTexCoord) * weight[0]; // current fragment's contribution
    for(int i = 1; i < 3; ++i) {
        result += texture(image, fragTexCoord + vec2(offset[i] * tex_offset.x, 0.0)) * weight[i];
        result += texture(image, fragTexCoord - vec2(offset[i] * tex_offset.x, 0.0)) * weight[i];
    }
    finalColor = result;
}
