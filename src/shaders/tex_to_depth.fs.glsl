#version 330 core

in vec2 fragTexCoord;

uniform sampler2D image;

void main() {
    float result = texture(image, fragTexCoord).r;
    gl_FragDepth = result;
}
