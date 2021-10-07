#include "ParticleSystem.hpp"

#include <array>
#include <glad/glad.h>
#include <imgui.h>
#include <numeric>

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

	m_simple_spawner_program = ShaderProgram(
		&Shader(shad_dir / "simple_spawner.comp", Shader::Type::Compute),
		1
	);


	glGenVertexArrays(1, &m_ico_draw_vao);
	

	// Generate particle buffers
	glGenBuffers(2, m_vbo_particle_buffers);
	glGenBuffers(2, m_draw_indirect_buffers);
	glGenBuffers(2, m_alive_particle_indices);
	glGenBuffers(1, &m_dead_particle_indices);

	for (uint32_t i = 0; i < 2; ++i) {
		// Initialise indirect draw buffer, and bind in 3
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_indirect_buffers[i]);
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

	m_spawner_config.pos = glm::vec3(5.0f);
	m_spawner_config.mean_lifetime = 2.0f;
	// Generate buffer config
	glGenBuffers(1, &m_system_config_bo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_system_config_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(ParticleSystemConfig) + sizeof(ParticleSpawnerConfig),
		nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SYSTEM_CONFIG, m_system_config_bo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	update_sytem_config();
	initialize_system();
}

void ParticleSystem::update()
{
	// Bind in compute shader as bindings 1 and 2
	// 1 is previous frame, and 2 is the next
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_IN, m_vbo_particle_buffers[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_OUT, m_vbo_particle_buffers[1 - m_flipflop_state]);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_INDIRECT_IN, m_draw_indirect_buffers[m_flipflop_state]);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_INDIRECT_OUT, m_draw_indirect_buffers[1 - m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_ALIVE_IN, m_alive_particle_indices[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_ALIVE_OUT, m_alive_particle_indices[1 - m_flipflop_state]);

	// Clear the atomic counter of alive particles
	glInvalidateBufferSubData(m_draw_indirect_buffers[1 - m_flipflop_state],
		offsetof(DrawElementsIndirectCommand, primCount),
		sizeof(uint32_t));
	glClearNamedBufferSubData(m_draw_indirect_buffers[1 - m_flipflop_state], GL_R32F,
		offsetof(DrawElementsIndirectCommand, primCount),
		sizeof(uint32_t), GL_RED, GL_FLOAT, nullptr);

	uint32_t num_particles_to_instantiate;
	{
		m_accum_particles_emmited += m_emmit_particles_per_second * 
			ImGui::GetIO().DeltaTime;
		float floor_part = std::floor(m_accum_particles_emmited);
		m_accum_particles_emmited -= floor_part;
		num_particles_to_instantiate = static_cast<uint32_t>(floor_part);
	}
	if (num_particles_to_instantiate != 0) {
		m_simple_spawner_program.use_program();
		glUniform1ui(1, num_particles_to_instantiate);
		glDispatchCompute(num_particles_to_instantiate / 32
			+ (num_particles_to_instantiate % 32 == 0 ? 0 : 1), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	// Start compute shader
	m_advect_compute_program.use_program();
	glUniform1f(0, std::min(ImGui::GetIO().DeltaTime, 1 / 60.0f));
	glDispatchCompute(m_system_config.max_particles / 32
		+ (m_system_config.max_particles % 32 == 0 ? 0 : 1)
		, 1, 1);

	// flip state
	m_flipflop_state = !m_flipflop_state;
}

void ParticleSystem::gl_render_particles() const
{
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	glBindVertexArray(m_ico_draw_vao);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_indirect_buffers[m_flipflop_state]);

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
	if (ImGui::TreeNode("Spawner Config")) {

		ImGui::InputFloat("Particles/Second", &m_emmit_particles_per_second, 1.0f, 10.0f);

		ImGui::TreePop();
	}

	ImGui::Separator();

	if (ImGui::Button("Commit config")) {
		update_sytem_config();
		initialize_system();
	}

	ImGui::PopID();

}

void ParticleSystem::initialize_system()
{
	m_flipflop_state = false;
	std::vector<Particle> particles(m_system_config.max_particles, { glm::vec3(5.0f) });
	for (uint32_t i = 0; i < m_system_config.max_particles; ++i) {
		particles[i].pos.x += (float)i;
		particles[i].pos.y += (float)(i) * 0.2f;
	}

	if (m_max_particles_in_buffers < m_system_config.max_particles) {
		for (uint32_t i = 0; i < 2; ++i) {
			// Reserve particle data
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo_particle_buffers[i]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * m_system_config.max_particles,
				nullptr, GL_DYNAMIC_DRAW);
			// Reserve particle indices
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_alive_particle_indices[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * m_system_config.max_particles,
				nullptr, GL_DYNAMIC_DRAW);
		}
		
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_dead_particle_indices);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * m_system_config.max_particles,
			nullptr, GL_DYNAMIC_DRAW);

		m_max_particles_in_buffers = m_system_config.max_particles;
	}

	// Initialize dead particles (all)
	{
		std::vector<uint32_t> dead_indices(m_system_config.max_particles);
		std::iota(dead_indices.rbegin(), dead_indices.rend(), 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_dead_particle_indices);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER,
			0, // offset
			sizeof(uint32_t) * m_system_config.max_particles, // size
			dead_indices.data());
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_DEAD, m_dead_particle_indices);

	glBindBuffer(GL_ARRAY_BUFFER, 0);


	// Initialise indirect draw buffer, and bind in 3, to use the atomic counter to track the particles that are alive
	for (uint32_t i = 0; i < 2; ++i) {
		glClearNamedBufferSubData(m_draw_indirect_buffers[i], GL_R32F,
			offsetof(DrawElementsIndirectCommand, primCount),
			sizeof(uint32_t), GL_RED, GL_FLOAT, nullptr);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3 + i, m_draw_indirect_buffers[i]);
	}
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	
	// Create VAO to draw. Only 1 to use the buffer to get the instances
	glBindVertexArray(m_ico_draw_vao);
	m_ico_mesh.gl_bind_to_vao();

	glBindVertexArray(0);
}

void ParticleSystem::update_sytem_config()
{
	glNamedBufferSubData(
		m_system_config_bo, // buffer name
		0, // offset
		sizeof(ParticleSystemConfig),	// size
		&m_system_config	// data
	);
	
	glNamedBufferSubData(
		m_system_config_bo, // buffer name
		sizeof(ParticleSystemConfig), // offset
		sizeof(ParticleSpawnerConfig),	// size
		&m_spawner_config	// data
	);

}
