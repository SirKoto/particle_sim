#version 430 core

out vec4 fragColor;

in vec3 tangent_world;
in vec3 p_world;

layout(location = 2) uniform vec3 eye_world;
layout(location = 3) uniform float alpha;
layout(location = 4) uniform vec3 c_spec;
layout(location = 5) uniform vec3 c_diff;

const vec3 light_dir = vec3(0.57735, -0.57735, -0.57735);


void main() {

    const vec3 v = normalize(eye_world - p_world);
    const vec3 h = normalize(v - light_dir);
    const vec3 t = normalize(tangent_world);

    const float cos_th = dot(t, h);
    const float k_spec = pow(sqrt(1.0 - cos_th * cos_th), alpha);

    const float cos_tl = dot(t, light_dir);
    const float k_diff = sqrt(1.0 - cos_tl * cos_tl);

    fragColor = vec4( k_diff * c_diff + k_spec * c_spec , 1.0);
}

