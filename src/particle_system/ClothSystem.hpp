#pragma once

#include "graphics/ShaderProgram.hpp"
#include "spring_types.in"
#include "intersections.comp.in"

class ClothSystem {
public:
	ClothSystem();

	ClothSystem(const ClothSystem&) = delete;
	ClothSystem& operator=(const ClothSystem&) = delete;

	void update(float time, float dt);

	void gl_render(const glm::mat4& proj_view, const glm::vec3& eye_world);

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
	// uint32_t m_patches_indices_bo;
	uint32_t m_forces_buffer;
	uint32_t m_original_lengths_buffer;
	uint32_t m_fixed_points_buffer;
	uint32_t m_particle_2_segments_list;
	uint32_t m_segments_list_buffer;

	ShaderProgram m_basic_draw_point;
	ShaderProgram m_advect_particle_program;
	ShaderProgram m_spring_force_program;

	uint32_t m_sphere_ssb;

	uint32_t m_segment_vao;

	Sphere m_sphere_head = { glm::vec3(2.0f, 5.0f, 5.0f), 1.0f };

	glm::uvec2 m_resolution_cloth = glm::uvec2(10, 10);

	bool m_draw_points = true;
	bool m_draw_lines = true;
	bool m_intersect_sphere = true;

	void initialize_system();
	void update_interaction_data();
	void update_system_config();
};