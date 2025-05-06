#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D depth;

float linearizeDepth(float d, float zNear, float zFar) {
    float z_n = 2.0 * d - 1.0;
    float lin = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
    return (lin - zNear) / (zFar - zNear);
}

void main() {
    float depth = texture(depth, fragTexCoord).r;
    finalColor = vec4(vec3(linearizeDepth(depth, 0.01, 10.0)), 1);
}
