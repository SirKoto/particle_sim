#version 430 core

out vec4 fragColor;

in vec3 normWorld;

const vec3 light_dir = vec3(0.57735, -0.57735, -0.57735);

void main() {
    vec3 color = vec3(.5,0.8,0.2);
    color = color * (0.2 + 0.8 * max(dot(normWorld, -light_dir), 0.0));
    fragColor = vec4(color, 1.0);
}
