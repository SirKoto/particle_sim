#version 430

layout ( quads, equal_spacing, ccw ) in;

layout(location = 1) uniform mat4 PV;

out vec3 normal_world;
out vec3 p_world;

// gl_in[0].gl_Position.xyz

vec3 b_spline_quadratic(float t, vec3 p0, vec3 p1, vec3 p2) {
    vec3 o = 0.5 * vec3(t*t - 2.0*t + 1.0, -2.0*t*t + 2.0*t +1.0, t*t);

    return p0 * o.x + p1 * o.y + p2 * o.z;
}

vec3 b_spline_quadratic_derivative(float t, vec3 p0, vec3 p1, vec3 p2) {
    vec3 o = vec3(t - 1.0, -2.0*t + 1.0, t);

    return p0 * o.x + p1 * o.y + p2 * o.z;
}

vec3 b_spline_surface_patch(vec2 uv, out vec3 normal) {
    vec3 p0 = b_spline_quadratic(uv.x, gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
    vec3 p1 = b_spline_quadratic(uv.x, gl_in[3].gl_Position.xyz, gl_in[4].gl_Position.xyz, gl_in[5].gl_Position.xyz);
    vec3 p2 = b_spline_quadratic(uv.x, gl_in[6].gl_Position.xyz, gl_in[7].gl_Position.xyz, gl_in[8].gl_Position.xyz);

    vec3 dp0du = b_spline_quadratic_derivative(uv.x, gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
    vec3 dp1du = b_spline_quadratic_derivative(uv.x, gl_in[3].gl_Position.xyz, gl_in[4].gl_Position.xyz, gl_in[5].gl_Position.xyz);
    vec3 dp2du = b_spline_quadratic_derivative(uv.x, gl_in[6].gl_Position.xyz, gl_in[7].gl_Position.xyz, gl_in[8].gl_Position.xyz);

    vec3 dpdu = b_spline_quadratic(uv.y, dp0du, dp1du, dp2du);
    vec3 dpdv = b_spline_quadratic_derivative(uv.y, p0, p1, p2);

    normal = normalize(cross(dpdu, dpdv));

    return b_spline_quadratic(uv.y, p0, p1, p2);
}

void main() {
    //float v = gl_TessCoord.y;
    //tangent_world = b_spline_quadratic_derivative(u);
    vec3 pos = b_spline_surface_patch(gl_TessCoord.xy, normal_world);
    p_world = pos;
    gl_Position = PV * vec4(pos, 1.0);
}
