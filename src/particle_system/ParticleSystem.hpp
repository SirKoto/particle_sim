#pragma once

#include "graphics/ShaderProgram.hpp"
#include "graphics/TriangleMesh.hpp"
#include "particle_types.in"
#include "intersections.comp.in"
#include <memory>


class ParticleSystem {
public:

	ParticleSystem();
	ParticleSystem(const ParticleSystem&) = delete;
	ParticleSystem& operator=(const ParticleSystem&) = delete;

	void update(float time, float dt);

	void gl_render_particles() const;

	void imgui_draw();

	void set_sphere(const glm::vec3& pos, float radius);
	void remove_sphere();

	void set_mesh(const TriangleMesh& mesh, const glm::mat4& transform);

private:
	TriangleMesh m_ico_mesh;
	uint32_t m_ico_draw_vao;

	bool m_flipflop_state = false;
	uint32_t m_vbo_particle_buffers[2];
	uint32_t m_alive_particle_indices[2];
	uint32_t m_dead_particle_indices, m_dead_particle_count;

	uint32_t m_draw_indirect_buffers[2];

	ShaderProgram m_advect_compute_program;
	ShaderProgram m_simple_spawner_program;

	ParticleSystemConfig m_system_config;
	ParticleSpawnerConfig m_spawner_config;
	uint32_t m_system_config_bo;

	float m_emmit_particles_per_second = 10.0f;
	float m_accum_particles_emmited = 0.0f;
	uint32_t m_max_particles_in_buffers = 0;

	uint32_t m_sphere_ssb;
	bool m_intersect_sphere_enabled = true;
	Sphere m_sphere;

	bool m_intersect_mesh_enabled = true;
	TriangleMesh m_intersect_mesh;

	void initialize_system();
	void update_sytem_config();
	void update_intersection_sphere();
	void update_intersection_mesh();
};