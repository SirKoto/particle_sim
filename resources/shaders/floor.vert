#version 430 core
layout(location = 0) in vec3 iPos;

layout(location = 0) uniform float floor_size;
layout(location = 1) uniform mat4 PV;

void main() {
    gl_Position = PV * vec4(iPos * vec3(floor_size, 1.0, floor_size), 1.0);
}
