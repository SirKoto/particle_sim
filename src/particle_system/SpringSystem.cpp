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

	m_spring_force_program = ShaderProgram(
		&Shader(shad_dir / "spring_forces.comp", Shader::Type::Compute), 1
	);

	glGenBuffers(2, m_vbo_particle_buffers);
	glGenBuffers(1, &m_system_config_bo);
	glGenBuffers(1, &m_spring_indices_bo);
	glGenBuffers(1, &m_sphere_ssb);
	glGenBuffers(1, &m_forces_buffer);
	glGenBuffers(1, &m_original_lengths_buffer);
	glGenBuffers(1, &m_fixed_points_buffer);
	glGenBuffers(1, &m_particle_2_segments_list);
	glGenBuffers(1, &m_segments_list_buffer);

	glGenVertexArrays(1, &m_segment_vao);

	// Bind indices
	glBindVertexArray(m_segment_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spring_indices_bo);
	glBindVertexArray(0);

	m_system_config.num_particles = 10;
	m_system_config.num_segments = m_system_config.num_particles - 1;
	m_system_config.k_v = 0.9999f;
	m_system_config.gravity = 9.8f;
	m_system_config.simulation_space_size = 10.0f;
	m_system_config.bounce = 0.5f;
	m_system_config.k_e = 20.f;
	m_system_config.k_d = 5.0f;
	m_system_config.particle_mass = 1.0f;
	m_system_config.num_fixed_particles = 1;

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
	update_intersection_sphere();
	update_interaction_data();
}

void SpringSystem::update(float time, float dt)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_IN, m_vbo_particle_buffers[m_flipflop_state]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLES_OUT, m_vbo_particle_buffers[1 - m_flipflop_state]);

	glClearNamedBufferSubData(m_forces_buffer, GL_R32F,
		0, sizeof(glm::vec4) * m_system_config.num_segments, GL_RED, GL_FLOAT, nullptr);
	
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	m_spring_force_program.use_program();
	glUniform1f(0, dt);
	glDispatchCompute(m_system_config.num_segments / 32
		+ (m_system_config.num_segments % 32 == 0 ? 0 : 1)
		, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

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
			2 * m_system_config.num_segments,
			GL_UNSIGNED_INT, nullptr);
	}
}

void SpringSystem::imgui_draw()
{
	ImGui::PushID("springSys");
	ImGui::Text("Spring System Config");
	bool update = false;
	update |= ImGui::DragFloat("Gravity", &m_system_config.gravity, 0.01f);
	//update |= ImGui::DragFloat("Particle size", &m_system_config.particle_size, 0.01f, 0.0f, 2.0f);
	update |= ImGui::InputFloat("Simulation space size", &m_system_config.simulation_space_size, 0.1f);
	update |= ImGui::InputFloat("Verlet damping", &m_system_config.k_v, 0.0001f, 0.0f, "%.5f");
	update |= ImGui::InputFloat("Bounciness", &m_system_config.bounce, 0.1f);

	update |= ImGui::DragFloat("K elastic", &m_system_config.k_e, 0.1f);
	update |= ImGui::DragFloat("K damping", &m_system_config.k_d, 0.1f);
	update |= ImGui::DragFloat("Particle mass", &m_system_config.particle_mass, 0.1f);

	if (update) {
		update_sytem_config();
	}

	ImGui::Text("Interaction:");
	if (ImGui::DragFloat3("Position", glm::value_ptr(m_interact_point), 0.01f)) {
		update_interaction_data();
	}

	ImGui::Separator();
	if(ImGui::Checkbox("Sphere collisions", &m_intersect_sphere)) {
		update_intersection_sphere();
	}

	ImGui::Checkbox("Draw Points", &m_draw_points);
	ImGui::Checkbox("Draw Lines", &m_draw_lines);
	ImGui::Separator();
	ImGui::Combo("Init system", reinterpret_cast<int*>(&m_init_system), "Rope\0");

	if (m_init_system == InitSystems::eRope) {
		ImGui::PushID("RopeInit");
		ImGui::DragFloat3("Rope direction", glm::value_ptr(m_rope_init_dir), 0.01f);
		ImGui::InputScalar("Num particles", ImGuiDataType_U32, &m_rope_init_num_particles);
		ImGui::InputFloat("Rope length", &m_rope_init_length, 0.1f);
		ImGui::InputScalar("Num fixed particles", ImGuiDataType_U32, &m_rope_init_num_fixed_particles);

		ImGui::PopID();
	}

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
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SEGMENT_INDICES, m_spring_indices_bo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_FORCES, m_forces_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SHAPE_SPHERE, m_sphere_ssb);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_ORIGINAL_LENGTHS, m_original_lengths_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_FIXED_POINTS, m_fixed_points_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_PARTICLE_TO_SEGMENTS_LIST, m_particle_2_segments_list);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_SEGMENTS_MAPPING_LIST, m_segments_list_buffer);
}

void SpringSystem::initialize_system()
{
	m_flipflop_state = false;


	for (uint32_t i = 0; i < 2; ++i) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vbo_particle_buffers[i]);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
			m_system_config.num_particles * sizeof(Particle),
			nullptr, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	switch (m_init_system)
	{
	case InitSystems::eRope:
		init_system_rope();
		break;
	default:
		break;
	}
	
	// force buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_forces_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(glm::vec4) * m_system_config.num_segments,
		nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glClearNamedBufferSubData(m_forces_buffer, GL_R32F,
		0, sizeof(glm::vec4) * m_system_config.num_segments, GL_RED, GL_FLOAT, nullptr);

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

void SpringSystem::update_intersection_sphere()
{
	m_advect_particle_program.use_program();
	if (m_intersect_sphere) {
		glUniform1ui(1, 1);
	}
	else {
		glUniform1ui(1, 0);
	}
	glUseProgram(0);
}

void SpringSystem::init_system_rope()
{
	const uint32_t num_particles = m_system_config.num_particles = m_rope_init_num_particles;
	m_system_config.num_segments = num_particles - 1;
	m_system_config.num_fixed_particles = m_rope_init_num_fixed_particles;

	std::vector<Particle> p(num_particles);
	const glm::vec3 dir = glm::normalize(m_rope_init_dir);
	float delta_x = m_rope_init_length / (float)m_rope_init_num_particles;
	for (uint32_t i = 0; i < num_particles; ++i) {
		p[i].pos = m_interact_point + dir * ((float)i * delta_x);
	}
	glNamedBufferData(
		m_vbo_particle_buffers[0], // buffer name
		num_particles * sizeof(Particle),	// size
		p.data(),	// data
		GL_DYNAMIC_DRAW
	);
	glNamedBufferData(
		m_vbo_particle_buffers[1], // buffer name
		num_particles * sizeof(Particle),	// size
		p.data(),	// data
		GL_DYNAMIC_DRAW
	);

	// Segment indices
	std::vector<glm::ivec2> indices(m_system_config.num_segments);
	for (uint32_t i = 0; i < m_system_config.num_segments; ++i) {
		indices[i] = glm::ivec2(i, i + 1);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spring_indices_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(glm::ivec2) * indices.size(),
		indices.data(), GL_STATIC_DRAW);

	// Segment lengths
	std::vector<float> original_lengths(m_system_config.num_segments);
	for (uint32_t i = 0; i < m_system_config.num_segments; ++i) {
		original_lengths[i] = glm::length(p[indices[i].x].pos - p[indices[i].y].pos);
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_original_lengths_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(float) * original_lengths.size(),
		original_lengths.data(), GL_STATIC_DRAW);

	// Point 2 segment
	std::vector<SegmentMapping> mappings;
	mappings.reserve(num_particles * 2 - 2);
	std::vector<Particle2SegmentsList> particle2segments_map(num_particles);
	for (uint32_t i = 0; i < num_particles; ++i) {
		uint32_t num = 0;
		uint32_t idx = (uint32_t)mappings.size();

		if (i != 0) {
			mappings.push_back({ i - 1, 1 });
			num += 1;
		}
		if (i != num_particles - 1) {
			mappings.push_back({ i, 0 });
			num += 1;
		}
		
		particle2segments_map[i].segment_mapping_idx = idx;
		particle2segments_map[i].num_segments = num;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particle_2_segments_list);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Particle2SegmentsList) * particle2segments_map.size(),
		particle2segments_map.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_segments_list_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(SegmentMapping) * mappings.size(),
		mappings.data(), GL_STATIC_DRAW);
	// Upload also fixed particles, modify original points
	for (uint32_t i = 0; i < m_rope_init_num_fixed_particles; ++i) {
		p[i].pos -= m_interact_point;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fixed_points_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Particle) * m_rope_init_num_fixed_particles,
		p.data(),
		GL_STATIC_DRAW);

}

void SpringSystem::update_interaction_data()
{
	m_advect_particle_program.use_program();
	glUniform3fv(2, 1, glm::value_ptr(m_interact_point));
}
