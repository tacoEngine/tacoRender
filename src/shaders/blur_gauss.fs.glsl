#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D image;
uniform int radius;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec4 result = texture(image, fragTexCoord) * weight[0]; // current fragment's contribution
    for(int i = 1; i < 5; ++i) {
        result += texture(image, fragTexCoord + vec2(i * tex_offset.y, 0.0)) * weight[i];
        result += texture(image, fragTexCoord - vec2(i * tex_offset.y, 0.0)) * weight[i];
    }
    finalColor = result;
}
