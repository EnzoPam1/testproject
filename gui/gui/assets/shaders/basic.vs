#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

out vec2 fragTexCoord;
out vec3 fragNormal;

uniform mat4 mvp;

void main() {
    fragTexCoord = vertexTexCoord;
    fragNormal = vertexNormal;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
