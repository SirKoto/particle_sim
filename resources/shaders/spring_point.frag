#version 430 core

out vec4 fragColor;

void main() {
    vec3 color = vec3(1,0.2,0.2);
    fragColor = vec4(color, 1.0);
}

