#version 430

layout ( isolines, equal_spacing, ccw ) in;

layout(location = 1) uniform mat4 PV;


vec3 plane_curve(float u)
{
    vec3 p = (2*u*u - 3*u + 1) * gl_in[0].gl_Position.xyz;
    p += (-4*u*u + 4*u) * gl_in[1].gl_Position.xyz;
    p += (2*u*u - u) * gl_in[2].gl_Position.xyz;

    return p;
}

vec3 b_spline_quadratic(float t) {
    vec3 o = 0.5 * vec3(t*t - 2.0*t + 1.0, -2.0*t*t + 2.0*t +1.0, t*t);

    return gl_in[0].gl_Position.xyz * o.x
        + gl_in[1].gl_Position.xyz * o.y 
        + gl_in[2].gl_Position.xyz * o.z;
}

void main() {
    float u = gl_TessCoord.x;
    //float v = gl_TessCoord.y;
    vec3 pos = b_spline_quadratic(u);
    gl_Position = PV * vec4(pos, 1.0);
}
