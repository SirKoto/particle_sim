#pragma once

#include "graphics/ShaderProgram.hpp"
#include "graphics/TriangleMesh.hpp"
#include "particle_types.in"


class ParticleSystem {
public:

	ParticleSystem();
	ParticleSystem(const ParticleSystem&) = delete;
	ParticleSystem& operator=(const ParticleSystem&) = delete;

	void update();

	void gl_render_particles() const;

	void imgui_draw();

private:
	TriangleMesh m_ico_mesh;
	uint32_t m_ico_draw_vao;

	bool m_flipflop_state = false;
	uint32_t m_vbo_particle_buffers[2];
	uint32_t m_alive_particle_indices[2];
	uint32_t m_dead_particle_indices;

	uint32_t m_draw_indirect_buffers[2];

	ShaderProgram m_advect_compute_program;
	ShaderProgram m_simple_spawner_program;

	ParticleSystemConfig m_system_config;
	uint32_t m_system_config_bo;

	uint32_t m_max_particles_in_buffers = 0;

	void initialize_system();
	void update_sytem_config();
};