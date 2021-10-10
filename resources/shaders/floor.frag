#version 430 core

out vec4 fragColor;


const vec3 light_dir = vec3(0.57735, -0.57735, -0.57735);

void main() {
    vec3 color = vec3(.9,0.9,0.7);
    color = color * (0.2 + 0.8 * abs(light_dir.y));
    fragColor = vec4(color, 1.0);
}
