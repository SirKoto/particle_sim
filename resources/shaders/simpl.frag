#version 430 core

out vec4 fragColor;

//in vec3 normWorld;
//in vec2 uv;

void main() {
    vec3 color = vec3(1,0,0);
    fragColor = vec4(color, 1.0);
}
