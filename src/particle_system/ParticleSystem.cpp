#include "ParticleSystem.hpp"

#include <array>
#include <glad/glad.h>
#include <imgui.h>

#include "graphics/my_gl_header.hpp"

ParticleSystem::ParticleSystem()
{
	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	m_ico_mesh = TriangleMesh(proj_dir / "resources/ply/icosahedron.ply");
	m_ico_mesh.upload_to_gpu();


	m_advect_compute_program = ShaderProgram(
		&Shader(shad_dir / "advect_particles.comp", Shader::Type::Compute),
		1
	);


	glGenVertexArrays(2, m_ico_draw_vaos);
	

	// Generate particle buffers
	glGenBuffers(2, m_vbo_particle_buffers);
	glGenBuffers(1, &m_atomic_num_particles_alive_bo);
	glGenBuffers(1, &m_draw_indirect_bo);
	{
		// Initialise indirect draw buffer, and bind in 3
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_indirect_bo);
		DrawElementsIndirectCommand command{
			(uint32_t)m_ico_mesh.get_faces().size() * 3, // num elements
			0, // instance count
			0, // first index
			0, // basevertex
			0 // baseinstance
		};
		glBufferData(GL_DRAW_INDIRECT_BUFFER,
			sizeof(DrawElementsIndirectCommand),
			&command, GL_DYNAMIC_DRAW);
	}

	// Setup configuration
	m_system_config.max_particles = 5;
	m_system_config.gravity = 9.8f;
	m_system_config.particle_size = 1.0e-1f;
	m_system_config.simulation_space_size = 10.0f;
	m_system_config.k_v = 0.9999f;
	m_system_config.bounce = 0.5f;
	glGenBuffers(1, &m_system_config_bo);
	update_sytem_config();

	initialize_system();
}

void ParticleSystem::update()
{
	// Bind in compute shader as bindings 1 and 2
	// 1 is previous frame, and 2 is the next
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo_particle_buffers[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_vbo_particle_buffers[1 - m_flipflop_state]);
	m_flipflop_state = !m_flipflop_state;


	m_advect_compute_program.use_program();
	glUniform1f(0, std::min(ImGui::GetIO().DeltaTime, 1 / 60.0f));
	glDispatchCompute(m_system_config.max_particles, 1, 1);
}

void ParticleSystem::gl_render_particles() const
{
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	glBindVertexArray(m_ico_draw_vaos[m_flipflop_state]);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_indirect_bo);

	glDrawElementsIndirect(GL_TRIANGLES,
		GL_UNSIGNED_INT,
		(void*)0);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	glBindVertexArray(0);


}

void ParticleSystem::imgui_draw()
{
	ImGui::PushID("Particlesystem");
	ImGui::Text("Particle System Config");
	ImGui::InputScalar("Max particles", ImGuiDataType_U32, &m_system_config.max_particles);
	ImGui::InputFloat("Gravity", &m_system_config.gravity, 0.1f);
	ImGui::InputFloat("Particle size", &m_system_config.particle_size, 0.1f);
	ImGui::InputFloat("Simulation space size", &m_system_config.simulation_space_size, 0.1f);
	ImGui::InputFloat("Verlet damping", &m_system_config.k_v, 0.0f, 0.0f, "%.5f");
	ImGui::InputFloat("Bounciness", &m_system_config.bounce, 0.1f);

	ImGui::Separator();

	if (ImGui::Button("Commit config")) {
		update_sytem_config();
		initialize_system();
	}

	ImGui::PopID();

}

void ParticleSystem::initialize_system()
{
	std::vector<Particle> particles(m_system_config.max_particles, { glm::vec3(5.0f) });
	for (uint32_t i = 0; i < m_system_config.max_particles; ++i) {
		particles[i].pos.x += (float)i;
	}

	// Initialize particle buffers
	for (uint32_t i = 0; i < 2; ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_particle_buffers[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * m_system_config.max_particles,
			particles.data(), GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	// Initialise indirect draw buffer, and bind in 3, to use the atomic counter to track the particles that are alvive
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_indirect_bo);
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offsetof(DrawElementsIndirectCommand, primCount),
		sizeof(uint32_t), &m_system_config.max_particles);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, m_draw_indirect_bo);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	
	// Create 2 vaos, for each vbo of particles
	for (uint32_t i = 0; i < 2; ++i) {
		glBindVertexArray(m_ico_draw_vaos[i]);
		m_ico_mesh.gl_bind_to_vao();
		glVertexAttribDivisor(0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_particle_buffers[i]);
		// offsets in location 1. Only set position as attrib
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			sizeof(Particle), (void*)offsetof(Particle, pos));
		glVertexAttribDivisor(1, 1);
	}


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void ParticleSystem::update_sytem_config()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_system_config_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleSystemConfig),
		&m_system_config, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_system_config_bo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
