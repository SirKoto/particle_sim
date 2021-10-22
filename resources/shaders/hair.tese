#version 430

layout ( isolines, equal_spacing, ccw ) in;

layout(location = 1) uniform mat4 PV;

out vec3 tangent_world;
out vec3 p_world;



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

vec3 b_spline_quadratic_derivative(float t) {
    vec3 o = vec3(t - 1.0, -2.0*t + 1.0, t);

    return gl_in[0].gl_Position.xyz * o.x
        + gl_in[1].gl_Position.xyz * o.y 
        + gl_in[2].gl_Position.xyz * o.z;
}

void main() {
    float u = gl_TessCoord.x;
    //float v = gl_TessCoord.y;
    tangent_world = b_spline_quadratic_derivative(u);
    vec3 pos = b_spline_quadratic(u);
    p_world = pos;
    gl_Position = PV * vec4(pos, 1.0);
}
