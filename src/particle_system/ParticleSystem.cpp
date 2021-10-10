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
	glGenBuffers(1, &m_dead_particle_count);
	glGenBuffers(1, &m_sphere_ssb);

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
	m_system_config.max_particles = 500;
	m_system_config.gravity = 9.8f;
	m_system_config.particle_size = 1.0e-1f;
	m_system_config.simulation_space_size = 10.0f;
	m_system_config.k_v = 0.9999f;
	m_system_config.bounce = 0.5f;

	m_spawner_config.pos = glm::vec3(5.0f);
	m_spawner_config.mean_lifetime = 2.0f;
	m_spawner_config.var_lifetime = 1.0f;
	m_spawner_config.particle_speed = 5.f;

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

	// Initialize shapes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Sphere),
		nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SHAPE_SPHERE, m_sphere_ssb);
	update_intersection_sphere();
	update_intersection_mesh();
}

void ParticleSystem::update(float time, float dt)
{
	// Bind in compute shader as bindings 1 and 2
	// 1 is previous frame, and 2 is the next
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_IN, m_vbo_particle_buffers[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_OUT, m_vbo_particle_buffers[1 - (uint32_t)m_flipflop_state]);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_ATOMIC_ALIVE_IN, m_draw_indirect_buffers[m_flipflop_state]);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_ATOMIC_ALIVE_OUT, m_draw_indirect_buffers[1 - (uint32_t)m_flipflop_state]);
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_ALIVE_LIST_IN, m_alive_particle_indices[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_ALIVE_LIST_OUT, m_alive_particle_indices[1 - (uint32_t)m_flipflop_state]);

	// Clear the atomic counter of alive particles
	glClearNamedBufferSubData(m_draw_indirect_buffers[1 - m_flipflop_state], GL_R32F,
		offsetof(DrawElementsIndirectCommand, primCount),
		sizeof(uint32_t), GL_RED, GL_FLOAT, nullptr);

	uint32_t num_particles_to_instantiate;
	{
		m_accum_particles_emmited += m_emmit_particles_per_second * dt;
		float floor_part = std::floor(m_accum_particles_emmited);
		m_accum_particles_emmited -= floor_part;
		num_particles_to_instantiate = static_cast<uint32_t>(floor_part);
	}
	if (num_particles_to_instantiate != 0) {
		m_simple_spawner_program.use_program();
		glUniform1f(0, time);
		glUniform1f(1, dt);
		glUniform1ui(2, num_particles_to_instantiate);
		glDispatchCompute(num_particles_to_instantiate / 32
			+ (num_particles_to_instantiate % 32 == 0 ? 0 : 1), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	// Start compute shader
	m_advect_compute_program.use_program();
	glUniform1f(0, dt);
	glDispatchCompute(m_system_config.max_particles / 32
		+ (m_system_config.max_particles % 32 == 0 ? 0 : 1)
		, 1, 1);

	// flip state
	m_flipflop_state = !m_flipflop_state;
}

void ParticleSystem::gl_render_particles() const
{
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

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
	bool update = false;

	ImGui::PushID("Particlesystem");
	ImGui::Text("Particle System Config");
	update |= ImGui::DragFloat("Gravity", &m_system_config.gravity, 0.01f);
	update |= ImGui::DragFloat("Particle size", &m_system_config.particle_size, 0.01f, 0.0f, 2.0f);
	update |= ImGui::InputFloat("Simulation space size", &m_system_config.simulation_space_size, 0.1f);
	update |= ImGui::InputFloat("Verlet damping", &m_system_config.k_v, 0.0001f, 0.0f, "%.5f");
	update |= ImGui::InputFloat("Bounciness", &m_system_config.bounce, 0.1f);

	ImGui::Separator();
	if (ImGui::TreeNode("Spawner Config")) {
		ImGui::DragFloat("Particles/Second", &m_emmit_particles_per_second, 1.0f, 10.0f);


		update |= ImGui::DragFloat3("Position", &m_spawner_config.pos.x, 0.01f);
		
		update |= ImGui::DragFloat("Initial Velocity", &m_spawner_config.particle_speed, 0.02f, 0.0f, FLT_MAX);

		update |= ImGui::DragFloat("Mean lifetime", &m_spawner_config.mean_lifetime, 0.2f, 0.0f, FLT_MAX);
		update |= ImGui::DragFloat("Var lifetime", &m_spawner_config.var_lifetime, 0.2f, 0.0f, FLT_MAX);


		ImGui::TreePop();
	}

	ImGui::Separator();
	ImGui::TextDisabled("Config system limit. Needs to reset simulation");
	bool reset = ImGui::InputScalar("Max particles", ImGuiDataType_U32, &m_system_config.max_particles);
	if (reset || ImGui::Button("Reset simulation")) {
		update_sytem_config();
		initialize_system();
	} else if (update) {
		update_sytem_config();
	}

	ImGui::Separator();
	if (ImGui::Checkbox("Sphere collisions", &m_intersect_sphere_enabled)) {
		update_intersection_sphere();
	}
	ImGui::Separator();
	if (ImGui::Checkbox("Mesh collisions", &m_intersect_mesh_enabled)) {
		update_intersection_mesh();
	}

	ImGui::PopID();

}

void ParticleSystem::set_sphere(const glm::vec3& pos, float radius)
{
	m_sphere.pos = pos;
	m_sphere.radius = radius;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Sphere), &m_sphere);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ParticleSystem::remove_sphere()
{
	m_intersect_sphere_enabled = false;
	update_intersection_sphere();
}

void ParticleSystem::set_mesh(const TriangleMesh& mesh, const glm::mat4& transform)
{
	m_intersect_mesh = TriangleMesh(mesh);
	m_intersect_mesh.apply_transform(transform);
	m_intersect_mesh.upload_to_gpu();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_MESH_VERTICES, m_intersect_mesh.get_vbo_vertices());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_MESH_INDICES, m_intersect_mesh.get_vbo_indices());
}

void ParticleSystem::initialize_system()
{
	// TODO
	m_accum_particles_emmited = 1.0f;

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
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// Reserve particle indices
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_alive_particle_indices[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * m_system_config.max_particles,
				nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_DEAD_LIST, m_dead_particle_indices);


	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_dead_particle_count);
	glNamedBufferData(m_dead_particle_count, sizeof(uint32_t), &m_system_config.max_particles, GL_STATIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_ATOMIC_DEAD, m_dead_particle_count);


	// Initialise indirect draw buffer, and bind in 3, to use the atomic counter to track the particles that are alive
	for (uint32_t i = 0; i < 2; ++i) {
		glClearNamedBufferSubData(m_draw_indirect_buffers[i], GL_R32F,
			offsetof(DrawElementsIndirectCommand, primCount),
			sizeof(uint32_t), GL_RED, GL_FLOAT, nullptr);
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

void ParticleSystem::update_intersection_sphere()
{
	m_advect_compute_program.use_program();
	if (m_intersect_sphere_enabled) {
		glUniform1ui(1, 1);
	}
	else {
		glUniform1ui(1, 0);
	}
	glUseProgram(0);
}

void ParticleSystem::update_intersection_mesh()
{
	m_advect_compute_program.use_program();
	if (m_intersect_mesh_enabled) {
		glUniform1ui(2, 1);
	}
	else {
		glUniform1ui(2, 0);
	}
	glUseProgram(0);
}
