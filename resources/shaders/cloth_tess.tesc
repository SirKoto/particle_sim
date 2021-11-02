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
layout (vertices = 9) out;

void main()
{
   // Regular quad
   gl_TessLevelOuter[0] = 6;
   gl_TessLevelOuter[1] = 6;
   gl_TessLevelOuter[2] = 6; 
   gl_TessLevelOuter[3] = 6; 

   gl_TessLevelInner[0] = 4;
   gl_TessLevelInner[1] = 4;


   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
