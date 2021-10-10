#version 430 core
layout(location = 0) in vec3 iPos;

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat4 PV;

out vec3 normWorld;

void main() {
    normWorld = iPos;
    gl_Position = PV * M * vec4(iPos, 1.0);
}
