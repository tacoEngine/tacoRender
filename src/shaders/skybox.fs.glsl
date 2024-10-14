#version 330 core

in vec3 fragTexCoord;

out vec4 finalColor;

uniform samplerCube skybox;

void main() {
    finalColor = texture(skybox, fragTexCoord + vec3(0, 0, 0));
}
