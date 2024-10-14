#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D source;
uniform float gamma = 2.2;

void main() {
    vec4 color = texture(source, fragTexCoord);
    vec3 corrected = pow(color.rgb, vec3(1.0 / gamma));
    finalColor = vec4(corrected, color.a);
}
