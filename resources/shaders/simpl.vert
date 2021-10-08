#version 430 core

#include "../shader_includes/particle_types.in"

layout(std430, binding = 0) buffer ConfigData{
    ParticleSystemConfig config;
};

layout(location = 0) in vec3 iPos;
//layout(location = 1) in vec3 iOffset;
//layout(location = 1) in vec3 iNorm;
//layout(location = 2) in vec2 iUv;
//layout(location = 3) in vec2 iOffsetXZ;


//layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat4 PV;

layout(std430, binding = BINDING_PARTICLES_OUT) buffer ParticleDataOut
{
    Particle particles[];
};

layout(std430, binding = BINDING_ALIVE_LIST_OUT) buffer ParticleIndicesAlive
{
    uint alive_particles_idx[];
};

//out vec3 normWorld;
//out vec2 uv;

void main() {
    
    //uv = iUv;
    //normWorld = (V * M * vec4(iNorm, 0.0)).xyz;
    /*
    if(gl_VertexID  < 6){
        normWorld = vec3(1,0,0);
    }
    else if(gl_VertexID < 2*6){
        normWorld = vec3(1,1,0);
    }
    else if(gl_VertexID < 3*6){
        normWorld = vec3(1,0,1);
    }
    else if(gl_VertexID < 4*6){
        normWorld = vec3(0,1,0);
    }
    else if(gl_VertexID < 5*6){
        normWorld = vec3(0,1,1);
    }
    else if(gl_VertexID < 6*6){
        normWorld = vec3(0,0,1);
    }
    */
    //vec4 posWorld = M * vec4(iPos, 1.0) + vec4(iOffsetXZ.x, 0, iOffsetXZ.y, 0);
    uint idx = alive_particles_idx[gl_InstanceID];
    gl_Position = PV * vec4(iPos * config.particle_size + particles[idx].pos, 1.0);

}
