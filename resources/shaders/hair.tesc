#version 430

/*
in gl_PerVertex
{
   vec4 gl_Position;
   float gl_PointSize;
   float gl_ClipDistance[];
}  gl_in [gl_MaxPatchVertices];
 
out gl_PerVertex
{
   vec4 gl_Position;
   float gl_PointSize;
   float gl_ClipDistance[];
} gl_out[];
*/

// Size of the output patch
layout (vertices = 3) out;

void main()
{
    gl_TessLevelOuter[0] = 1; // paralel lines to generate
    gl_TessLevelOuter[1] = 16; // Subdivision
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
