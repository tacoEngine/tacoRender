#version 330 core

in vec3 vertexPosition;

out vec3 fragTexCoord;

uniform mat4 matModel;
uniform mat4 matProjection;
uniform mat4 matView;

void main() {
    mat4 rotView = mat4(mat3(matView));
    fragTexCoord = vertexPosition;
    gl_Position = (matProjection * rotView * vec4(vertexPosition, 1.0)).xyww;
}
