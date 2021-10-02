#include "ParticleSystem.hpp"

#include <array>
#include <glad/glad.h>
#include <imgui.h>

#include "particle_types.in"

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


	glGenVertexArrays(1, &m_ico_draw_vao);
	

	// Generate particle buffers
	glGenBuffers(sizeof(m_vbo_particle_buffer) / sizeof(*m_vbo_particle_buffer), m_vbo_particle_buffer);
	glGenBuffers(1, &m_atomic_num_particles_alive_bo);
	initialize_system();
}

void ParticleSystem::update()
{
	m_advect_compute_program.use_program();
	glDispatchCompute(m_max_particles, 1, 1);
}

void ParticleSystem::gl_render_particles() const
{
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	glBindVertexArray(m_ico_draw_vao);

	// glDrawElements(GL_TRIANGLES, (GLsizei)m_ico_mesh.get_faces().size() * 3, GL_UNSIGNED_INT, nullptr);
	glDrawElementsInstanced(GL_TRIANGLES,
		(GLsizei)m_ico_mesh.get_faces().size() * 3, // num elements
		GL_UNSIGNED_INT, // type
		nullptr, // indices, already in elements buffer
		m_max_particles // instance count
	);
}

void ParticleSystem::imgui_draw()
{
	ImGui::Text("Particle System Config");
	ImGui::Separator();
}

void ParticleSystem::initialize_system()
{
	std::vector<Particle> particles(m_max_particles, { glm::vec3(0.0f) });
	for (uint32_t i = 0; i < m_max_particles; ++i) {
		particles[i].pos.x += (float)i;
	}

	// Initialize particle buffers
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_particle_buffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * m_max_particles, particles.data(), GL_DYNAMIC_DRAW);
	// Bind in compute shader as binding 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_vbo_particle_buffer[0]);

	// Initialize number of alive particles at the beginning
	// TODO: set zero
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomic_num_particles_alive_bo);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), &m_max_particles, GL_DYNAMIC_DRAW);
	// Binding 2
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_atomic_num_particles_alive_bo);


	glBindVertexArray(m_ico_draw_vao);
	m_ico_mesh.gl_bind_to_vao();
	glVertexAttribDivisor(0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_particle_buffer[0]);


	// offsets in location 1
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
		sizeof(Particle), (void*)offsetof(Particle, pos));
	glVertexAttribDivisor(1, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
