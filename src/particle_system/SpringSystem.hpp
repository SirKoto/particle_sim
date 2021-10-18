#pragma once

#include "graphics/ShaderProgram.hpp"
#include "spring_types.in"
#include "intersections.comp.in"

class SpringSystem {
public:

	SpringSystem();
	SpringSystem(const SpringSystem&) = delete;
	SpringSystem& operator=(const SpringSystem&) = delete;

	void update(float time, float dt);

	void gl_render(const glm::mat4& proj_view);

	void imgui_draw();

	void set_sphere(const glm::vec3& pos, float radius);

	float get_simulation_space_size() const { return m_system_config.simulation_space_size; }

	void reset_bindings() const;

private:

	spring::SpringSystemConfig m_system_config;
	uint32_t m_system_config_bo;

	bool m_flipflop_state = false;
	uint32_t m_vbo_particle_buffers[2];
	uint32_t m_spring_indices_bo;
	uint32_t m_forces_buffer;
	uint32_t m_original_lengths_buffer;
	uint32_t m_fixed_points_buffer;


	ShaderProgram m_basic_draw_point;
	ShaderProgram m_advect_particle_program;
	ShaderProgram m_spring_force_program;

	uint32_t m_segment_vao;

	uint32_t m_sphere_ssb;
	//bool m_intersect_sphere_enabled = true;
	Sphere m_sphere;

	bool m_draw_points = true;
	bool m_draw_lines = true;
	bool m_intersect_sphere = true;

	enum class InitSystems {
		eRope = 0,
	};

	InitSystems m_init_system = InitSystems::eRope;

	glm::vec3 m_interact_point = glm::vec3(2.0f, 5.0f, 5.0f);

	glm::vec3 m_rope_init_dir = glm::vec3(1.0f, 0.0f, 0.0f);
	uint32_t m_rope_init_num_particles = 20;
	float m_rope_init_length = 5.0f;
	uint32_t m_rope_init_num_fixed_particles = 1;

	void initialize_system();
	void update_sytem_config();
	void update_intersection_sphere();

	void init_system_rope();
	void update_interaction_data();
};