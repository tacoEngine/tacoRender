#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D source;
uniform float gamma = 2.2;

void main() {
    vec4 color = texture(source, fragTexCoord);
    vec3 mapped = color.rgb / (color.rgb + vec3(1));
    finalColor = vec4(mapped, color.a);
}
