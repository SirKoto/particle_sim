#pragma once

#include "graphics/ShaderProgram.hpp"
#include "spring_types.in"
#include "intersections.comp.in"
#include "graphics/TriangleMesh.hpp"
#include <glm/gtc/quaternion.hpp>

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
	uint32_t m_patches_indices_bo;
	uint32_t m_forces_buffer;
	uint32_t m_original_lengths_buffer;
	uint32_t m_fixed_points_buffer;
	uint32_t m_particle_2_segments_list;
	uint32_t m_segments_list_buffer;


	ShaderProgram m_basic_draw_point;
	ShaderProgram m_hair_draw_program;
	ShaderProgram m_advect_particle_program;
	ShaderProgram m_spring_force_program;

	uint32_t m_segment_vao;
	uint32_t m_patches_vao;

	uint32_t m_num_elements_patches = 0;

	uint32_t m_sphere_ssb;
	//bool m_intersect_sphere_enabled = true;
	Sphere m_sphere;
	Sphere m_sphere_head = { glm::vec3(2.0f, 5.0f, 5.0f), 1.0f};
	glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 m_rotation_axis = glm::vec3(1.0f, 0.0f, 0.0f);
	float m_rotation_rate = 0.01f;

	bool m_draw_points = true;
	bool m_draw_lines = true;
	bool m_intersect_sphere = true;

	enum class InitSystems {
		eRope = 0,
		eSphere = 1,
	};

	enum class DrawMode {
		ePolylines = 0,
		eTessellation = 1,
	};

	DrawMode m_draw_mode = DrawMode::ePolylines;
	InitSystems m_init_system = InitSystems::eRope;
	bool m_head_sphere_enabled = false;

	glm::vec3 m_rope_init_dir = glm::vec3(1.0f, 0.0f, 0.0f);
	uint32_t m_rope_init_num_particles = 20;
	float m_rope_init_length = 5.0f;
	uint32_t m_rope_init_num_fixed_particles = 1;

	uint32_t m_sphere_init_num_hairs = 100;
	uint32_t m_sphere_init_particles_per_strand = 10;
	float m_hair_length = 1.0f;

	uint32_t m_sphere_vao;
	ShaderProgram m_sphere_draw_program;
	TriangleMesh m_sphere_mesh;

	void initialize_system();
	void update_sytem_config();
	void update_intersection_sphere();

	void init_system_rope();
	void init_system_sphere();
	void update_interaction_data();

};