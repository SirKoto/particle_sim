#version 430 core

#include "../shader_includes/spring_types.in"


layout(location = 1) uniform mat4 PV;

layout(std430, binding = BINDING_PARTICLES_OUT) buffer ParticleDataOut
{
    Particle particles[];
};

void main() {
    gl_Position = PV * vec4(particles[gl_VertexID].pos, 1.0);
}
