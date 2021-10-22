#version 430 core

#include "../shader_includes/spring_types.in"


layout(std430, binding = BINDING_PARTICLES_OUT) buffer ParticleDataOut
{
    Particle particles[];
};

void main() {
    gl_Position = vec4(particles[gl_VertexID].pos, 1.0);
}
