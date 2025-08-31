#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D image;
uniform vec2 image_size;
uniform uint horizontal;

const float MAX_DISTANCE = 0.001f;

void main() {
    vec2 tex_offset = 1.0 / image_size;
    vec4 center_sample = texture(image, fragTexCoord);
    vec3 result = center_sample.rgb;
    float sample_count = 1.0f;
    if (horizontal == 1u) {
        for (int i = 1; i <= 4; i++) {
            vec4 right_sample = texture(image, fragTexCoord + vec2(i * tex_offset.x, 0.0));
            vec4 left_sample = texture(image, fragTexCoord - vec2(i * tex_offset.x, 0.0));

            if (distance(center_sample.w, right_sample.w) < MAX_DISTANCE) {
                result += right_sample.rgb;
                sample_count += 1.0f;
            }
            if (distance(center_sample.w, left_sample.w) < MAX_DISTANCE) {
                result += left_sample.rgb;
                sample_count += 1.0f;
            }
        }
    } else {
        for (int i = 1; i <= 4; i++) {
            vec4 right_sample = texture(image, fragTexCoord + vec2(0.0, i * tex_offset.y));
            vec4 left_sample = texture(image, fragTexCoord - vec2(0.0, i * tex_offset.y));

            if (distance(center_sample.w, right_sample.w) < MAX_DISTANCE) {
                result += right_sample.rgb;
                sample_count += 1.0f;
            }
            if (distance(center_sample.w, left_sample.w) < MAX_DISTANCE) {
                result += left_sample.rgb;
                sample_count += 1.0f;
            }
        }
    }
    finalColor = vec4(result / sample_count, center_sample.w);
}
