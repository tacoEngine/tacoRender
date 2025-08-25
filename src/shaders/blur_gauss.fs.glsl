#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D image;
uniform vec2 image_size;
uniform uint horizontal;

const float WEIGHTS[3] = float[](0.4026199469, 0.2442013420, 0.0544886846);

void main() {
    vec2 tex_offset = 1.0 / image_size;
    vec3 result = texture(image, fragTexCoord).rgb * WEIGHTS[0];
    if (horizontal == 1u) {
        for (int i = 1; i < 3; i++) {
            result += texture(image, fragTexCoord + vec2(i * tex_offset.x, 0.0)).rgb * WEIGHTS[i];
            result += texture(image, fragTexCoord - vec2(i * tex_offset.x, 0.0)).rgb * WEIGHTS[i];
        }
    } else {
        for (int i = 1; i < 3; i++) {
            result += texture(image, fragTexCoord + vec2(0.0, i * tex_offset.y)).rgb * WEIGHTS[i];
            result += texture(image, fragTexCoord - vec2(0.0, i * tex_offset.y)).rgb * WEIGHTS[i];
        }
    }
    finalColor = vec4(result, 1);
}
