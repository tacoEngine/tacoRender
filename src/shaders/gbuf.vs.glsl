#version 330 core

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec3 vertexTangent;
in vec2 vertexTexCoord2;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;
out vec3 fragTangent;
out vec2 fragTexCoord2;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

void main() {
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = mat3(matNormal) * vertexNormal;
    fragColor = vertexColor;
    fragTangent = vertexTangent;
    fragTexCoord2 = vertexTexCoord2;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
