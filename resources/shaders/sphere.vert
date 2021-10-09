#version 430 core
layout(location = 0) in vec3 iPos;

layout(location = 0) uniform vec3 pos;
layout(location = 1) uniform float radius;
layout(location = 2) uniform mat4 PV;

out vec3 normWorld;

void main() {
    normWorld = iPos;
    gl_Position = PV * vec4(iPos * radius + pos, 1.0);
}
