#include "SpringSystem.hpp"

#include <imgui.h>
#include <glad/glad.h>
#include <array>
#include <glm/gtc/type_ptr.hpp>


using namespace spring;

SpringSystem::SpringSystem()
{
	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	std::array<Shader, 2> particle_shaders = {
		Shader((shad_dir / "spring_point.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "spring_point.frag"), Shader::Type::Fragment)
	};

	m_basic_draw_point = ShaderProgram(particle_shaders.data(), (uint32_t)particle_shaders.size());

	m_advect_particle_program = ShaderProgram(
		&Shader(shad_dir / "advect_particles_springs.comp", Shader::Type::Compute), 1
	);

	glGenBuffers(2, m_vbo_particle_buffers);
	glGenBuffers(1, &m_system_config_bo);
	glGenBuffers(1, &m_spring_indices_bo);
	glGenBuffers(1, &m_sphere_ssb);

	glGenVertexArrays(1, &m_segment_vao);

	// Bind indices
	glBindVertexArray(m_segment_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spring_indices_bo);
	glBindVertexArray(0);

	m_system_config.num_particles = 10;
	m_system_config.k_v = 0.9999f;
	m_system_config.gravity = 9.8f;
	m_system_config.simulation_space_size = 10.0f;
	m_system_config.bounce = 0.5f,
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_system_config_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(SpringSystemConfig),
		nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Initialize shapes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Sphere),
		nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	initialize_system();
}

void SpringSystem::update(float time, float dt)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_IN, m_vbo_particle_buffers[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_OUT, m_vbo_particle_buffers[1 - m_flipflop_state]);


	m_advect_particle_program.use_program();
	glUniform1f(0, dt);
	glDispatchCompute(m_system_config.num_particles / 32
		+ (m_system_config.num_particles % 32 == 0 ? 0 : 1)
		, 1, 1);

	// flip state
	m_flipflop_state = !m_flipflop_state;
}

void SpringSystem::gl_render(const glm::mat4& proj_view)
{

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	if (m_draw_points || m_draw_lines) {
		m_basic_draw_point.use_program();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(proj_view));
	}
	if (m_draw_points) {
		glPointSize(10.0f);
		glDrawArrays(GL_POINTS, 0, m_system_config.num_particles);
	}
	if (m_draw_lines) {
		glBindVertexArray(m_segment_vao);
		glDrawElements(GL_LINES,
			2 * (m_system_config.num_particles - 1),
			GL_UNSIGNED_INT, nullptr);
	}
}

void SpringSystem::imgui_draw()
{
	ImGui::PushID("springSys");
	ImGui::Text("Spring System Config");

	ImGui::Checkbox("Draw Points", &m_draw_points);
	ImGui::Checkbox("Draw Lines", &m_draw_lines);
	ImGui::Separator();

	if (ImGui::Button("Reset")) {
		initialize_system();
	}

	ImGui::PopID();
}

void SpringSystem::set_sphere(const glm::vec3& pos, float radius)
{
	m_sphere.pos = pos;
	m_sphere.radius = radius;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Sphere), &m_sphere);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SpringSystem::reset_bindings() const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SYSTEM_CONFIG, m_system_config_bo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_IN, m_vbo_particle_buffers[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_OUT, m_vbo_particle_buffers[1]);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SHAPE_SPHERE, m_sphere_ssb);
}

void SpringSystem::initialize_system()
{
	m_flipflop_state = false;

	uint32_t num_particles = m_system_config.num_particles;

	for (uint32_t i = 0; i < 2; ++i) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vbo_particle_buffers[i]);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
			m_system_config.num_particles * sizeof(Particle),
			nullptr, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	std::vector<Particle> p(num_particles);
	for (uint32_t i = 0; i < num_particles; ++i) {
		p[i].pos = glm::vec3(2.0f + (float)i, 5.0f, 5.0f);
	}
	glNamedBufferSubData(
		m_vbo_particle_buffers[1], // buffer name
		0, // offset
		num_particles * sizeof(Particle),	// size
		p.data()	// data
	);
	glNamedBufferSubData(
		m_vbo_particle_buffers[0], // buffer name
		0, // offset
		num_particles * sizeof(Particle),	// size
		p.data()	// data
	);

	// Segment indices
	std::vector<glm::ivec2> indices(m_system_config.num_particles - 1);
	for (uint32_t i = 0; i < m_system_config.num_particles - 1; ++i) {
		indices[i] = glm::ivec2(i, i + 1);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spring_indices_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(glm::ivec2) * indices.size(),
		indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	update_sytem_config();
	reset_bindings();
}

void SpringSystem::update_sytem_config()
{
	glNamedBufferSubData(
		m_system_config_bo, // buffer name
		0, // offset
		sizeof(SpringSystemConfig),	// size
		&m_system_config	// data
	);
}
