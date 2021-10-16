#pragma once

#include "graphics/ShaderProgram.hpp"
#include "particle_types.in"
#include "intersections.comp.in"

class SpringSystem {
public:

	SpringSystem();
	SpringSystem(const SpringSystem&) = delete;
	SpringSystem& operator=(const SpringSystem&) = delete;

	void update(float time, float dt);

	void gl_render_triangle_ribbons();

	void imgui_draw();

	float get_simulation_space_size() const { return 5.0f; }

	void reset_bindings() const;


};