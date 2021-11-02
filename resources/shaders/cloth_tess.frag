#version 430 core

out vec4 fragColor;

in vec3 normal_world;
in vec3 p_world;

layout(location = 2) uniform vec3 eye_world;
layout(location = 3) uniform float alpha;
layout(location = 4) uniform vec3 c_spec;
layout(location = 5) uniform vec3 c_diff;

const vec3 light_dir = vec3(0.57735, -0.57735, -0.57735);


void main() {

    const vec3 v = normalize(eye_world - p_world);

    const vec3 normal = normalize(normal_world);

    const float k_spec = pow(abs(dot(light_dir, reflect(-v, normal))), alpha);

    const float k_diff = abs(dot(light_dir, normal));

    fragColor = vec4( k_diff * c_diff + k_spec * c_spec , 1.0);
    
}

