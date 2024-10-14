#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D texture1;

void main() {
    vec4 texel0 = texture(texture0, fragTexCoord);
    vec4 texel1 = texture(texture1, fragTexCoord);
    finalColor = texel0 + texel1;
}
