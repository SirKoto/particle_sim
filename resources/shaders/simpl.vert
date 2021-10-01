#version 430 core

#include "../shader_includes/particle_types.in"

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iOffset;
//layout(location = 1) in vec3 iNorm;
//layout(location = 2) in vec2 iUv;
//layout(location = 3) in vec2 iOffsetXZ;


//layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat4 PV;

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
    gl_Position = PV * vec4(iPos + iOffset, 1.0);

}
