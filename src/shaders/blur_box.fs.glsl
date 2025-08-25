#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D image;
uniform vec2 image_size;
uniform uint horizontal;

void main() {
    vec2 tex_offset = 1.0 / image_size;
    vec3 result = texture(image, fragTexCoord).rgb;
    if (horizontal == 1u) {
        for (int i = 1; i <= 4; i++) {
            result += texture(image, fragTexCoord + vec2(i * tex_offset.x, 0.0)).rgb;
            result += texture(image, fragTexCoord - vec2(i * tex_offset.x, 0.0)).rgb;
        }
    } else {
        for (int i = 1; i <= 4; i++) {
            result += texture(image, fragTexCoord + vec2(0.0, i * tex_offset.y)).rgb;
            result += texture(image, fragTexCoord - vec2(0.0, i * tex_offset.y)).rgb;
        }
    }
    finalColor = vec4(result / 9, 1);
}
