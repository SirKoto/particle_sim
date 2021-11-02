#include "ClothSystem.hpp"

#include <imgui.h>
#include <glad/glad.h>
#include <array>
#include <glm/gtc/type_ptr.hpp>

using namespace spring;

ClothSystem::ClothSystem()
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

	std::array<Shader, 4> tess_shaders = {
		Shader((shad_dir / "cloth_tess.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "cloth_tess.frag"), Shader::Type::Fragment),
		Shader((shad_dir / "cloth_tess.tesc"), Shader::Type::TessellationControl),
		Shader((shad_dir / "cloth_tess.tese"), Shader::Type::TessellationEvaluation)
	};
	m_tessellation_program = ShaderProgram(tess_shaders.data(), (uint32_t)tess_shaders.size());


	glGenBuffers(2, m_vbo_particle_buffers);
	glGenBuffers(1, &m_system_config_bo);
	glGenBuffers(1, &m_spring_indices_bo);
	glGenBuffers(1, &m_patches_indices_bo);
	glGenBuffers(1, &m_sphere_ssb);
	glGenBuffers(1, &m_forces_buffer);
	glGenBuffers(1, &m_original_lengths_buffer);
	glGenBuffers(1, &m_fixed_points_buffer);
	glGenBuffers(1, &m_particle_2_segments_list);
	glGenBuffers(1, &m_segments_list_buffer);

	glGenVertexArrays(1, &m_segment_vao);
	glGenVertexArrays(1, &m_patches_vao);

	// Bind indices
	glBindVertexArray(m_segment_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spring_indices_bo);
	glBindVertexArray(m_patches_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patches_indices_bo);
	glBindVertexArray(0);


	// Init config
	m_system_config.num_particles = 10;
	m_system_config.num_segments = m_system_config.num_particles - 1;
	m_system_config.k_v = 0.9999f;
	m_system_config.gravity = 9.8f;
	m_system_config.simulation_space_size = 10.0f;
	m_system_config.bounce = 0.5f;
	m_system_config.friction = 0.02f;
	m_system_config.k_e = 2000.f;
	m_system_config.k_d = 25.0f;
	m_system_config.particle_mass = 1.0f;
	m_system_config.num_fixed_particles = 1;
	m_system_config.num_particles_per_strand = 0;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_system_config_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(SpringSystemConfig),
		nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Initialize shapes
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		2 * sizeof(Sphere),
		nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	initialize_system();
	update_interaction_data();
}

void ClothSystem::update(float time, float dt)
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
	glm::quat q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glUniform4fv(3, 1, glm::value_ptr(q));
	glDispatchCompute(m_system_config.num_particles / 32
		+ (m_system_config.num_particles % 32 == 0 ? 0 : 1)
		, 1, 1);

	// flip state
	m_flipflop_state = !m_flipflop_state;
}

void ClothSystem::gl_render(const glm::mat4& proj_view, const glm::vec3& eye_world)
{
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	if (m_draw_mode == DrawMode::ePolylines) {
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
	else if (m_draw_mode == DrawMode::eTessellation) {
		m_tessellation_program.use_program();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(proj_view));
		glUniform3fv(2, 1, glm::value_ptr(eye_world));
		glUniform1f(3, m_specular_alpha);
		glUniform3fv(4, 1, glm::value_ptr(m_specular));
		glUniform3fv(5, 1, glm::value_ptr(m_diffuse));

		glPatchParameteri(GL_PATCH_VERTICES, 9);

		glBindVertexArray(m_patches_vao);
		
		glDisable(GL_CULL_FACE);
		glDrawElements(GL_PATCHES,
			m_num_elements_patches,
			GL_UNSIGNED_INT, nullptr);
		
		glEnable(GL_CULL_FACE);

		if (m_draw_points) {
			m_basic_draw_point.use_program();
			glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(proj_view));
			glPointSize(10.0f);
			glDrawArrays(GL_POINTS, 0, m_system_config.num_particles);
		}
	}
	glBindVertexArray(0);
}

void ClothSystem::imgui_draw()
{
	ImGui::PushID("clothSys");
	ImGui::Text("Cloth System Config");

	bool update = false;
	update |= ImGui::DragFloat("Gravity", &m_system_config.gravity, 0.01f);
	//update |= ImGui::DragFloat("Particle size", &m_system_config.particle_size, 0.01f, 0.0f, 2.0f);
	update |= ImGui::InputFloat("Simulation space size", &m_system_config.simulation_space_size, 0.1f);
	update |= ImGui::InputFloat("Verlet damping", &m_system_config.k_v, 0.0001f, 0.0f, "%.5f");
	update |= ImGui::InputFloat("Bounciness", &m_system_config.bounce, 0.1f);
	update |= ImGui::InputFloat("Friction", &m_system_config.friction, 0.01f);

	update |= ImGui::DragFloat("K elastic", &m_system_config.k_e, 0.1f);
	update |= ImGui::DragFloat("K damping", &m_system_config.k_d, 0.1f);
	update |= ImGui::DragFloat("Particle mass", &m_system_config.particle_mass, 0.1f);

	if (update) {
		update_system_config();
	}

	ImGui::Separator();

	ImGui::Text("Interaction:");
	if (ImGui::DragFloat3("Position", glm::value_ptr(m_sphere_head.pos), 0.01f)) {
		update_interaction_data();
	}
	
	if (ImGui::Checkbox("Sphere collisions", &m_intersect_sphere)) {
		update_interaction_data();
	}

	ImGui::Separator();

	ImGui::Combo("Draw mode", (int*)&m_draw_mode, "Polyline\0Tessellation");

	if (m_draw_mode == DrawMode::ePolylines) {
		ImGui::PushID("polyline");

		ImGui::Checkbox("Draw Points", &m_draw_points);
		ImGui::Checkbox("Draw Lines", &m_draw_lines);

		ImGui::PopID();
	}
	else if (m_draw_mode == DrawMode::eTessellation) {
		ImGui::PushID("Tessellation");

		ImGui::ColorEdit3("Diffuse color", glm::value_ptr(m_diffuse));
		ImGui::ColorEdit3("Specular color", glm::value_ptr(m_specular));
		ImGui::InputFloat("Alpha specular", &m_specular_alpha, 1.0f);

		ImGui::Checkbox("Draw Points", &m_draw_points);

		ImGui::PopID();
	}

	ImGui::Separator();

	ImGui::InputInt2("Resolution", (int*)glm::value_ptr(m_resolution_cloth));

	if (ImGui::Button("Reset")) {
		initialize_system();
	}

	ImGui::PopID();
}


void ClothSystem::initialize_system()
{
	m_flipflop_state = false;

	// TODO
	{
		for (uint32_t i = 0; i < 2; ++i) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vbo_particle_buffers[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				m_system_config.num_particles * sizeof(Particle),
				nullptr, GL_DYNAMIC_DRAW);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glm::vec2 cloth_size = glm::vec2(3.0f);
		glm::vec3 rope_init_dir = glm::vec3(1.0f, 0.0f, 0.0f);

		{
			const uint32_t num_particles = m_system_config.num_particles = m_resolution_cloth.x * m_resolution_cloth.y;
			// TODO: m_system_config.num_segments = num_particles - 1;
			m_system_config.num_fixed_particles = m_resolution_cloth.x;
			m_system_config.num_particles_per_strand = 0; //todo: delete

			std::vector<Particle> p; p.reserve(num_particles);
			// const glm::vec3 dir = glm::normalize(rope_init_dir);
			float delta_x = cloth_size.x / (float)m_resolution_cloth.x;
			float delta_y = cloth_size.y / (float)m_resolution_cloth.y;
			for (uint32_t j = 0; j < m_resolution_cloth.y; ++j) {
				for (uint32_t i = 0; i < m_resolution_cloth.x; ++i) {
					p.push_back({});
					p.back().pos = m_sphere_head.pos + glm::vec3((float)i * delta_x, 0.0f, (float)j * delta_y);
				}
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
			std::vector<glm::ivec2> indices;
			std::vector<std::vector<SegmentMapping>> mappings_buffer(num_particles);
			uint32_t total_mappings = 0;
			for (uint32_t j = 0; j < m_resolution_cloth.y; ++j) {
				for (uint32_t i = 0; i < m_resolution_cloth.x; ++i) {
					uint32_t base = j * m_resolution_cloth.x + i;

					if (i != m_resolution_cloth.x - 1) {
						uint32_t right = j * m_resolution_cloth.x + i + 1;

						uint32_t idx = (uint32_t)indices.size();
						indices.push_back(glm::ivec2(base, right));
						mappings_buffer[base].push_back({ idx, 0 });
						mappings_buffer[right].push_back({ idx, 1 });

						total_mappings += 2;
					}
					if (j != m_resolution_cloth.y - 1) {
						uint32_t up = (j + 1) * m_resolution_cloth.x + i;

						uint32_t idx = (uint32_t)indices.size();
						indices.push_back(glm::ivec2(base, up));

						mappings_buffer[base].push_back({ idx, 0 });
						mappings_buffer[up].push_back({ idx, 1 });

						total_mappings += 2;

					}
				}
			}

			std::vector<Particle2SegmentsList> particle2segments_map(num_particles);
			std::vector<SegmentMapping> mappings;
			mappings.reserve(total_mappings);
			for (uint32_t i = 0; i < num_particles; ++i) {
				uint32_t idx = (uint32_t)mappings.size();
				mappings.insert(std::end(mappings),
					std::begin(mappings_buffer[i]), std::end(mappings_buffer[i]));

				particle2segments_map[i] = { idx, (uint32_t)mappings_buffer[i].size() };
			}


			m_system_config.num_segments = (uint32_t)indices.size();

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spring_indices_bo);
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				sizeof(glm::ivec2) * indices.size(),
				indices.data(), GL_STATIC_DRAW);

			// Patch indices
			std::vector<int32_t> patch_indices; 
			patch_indices.reserve(m_resolution_cloth.x * m_resolution_cloth.y * 9);
			for (int32_t j = 0; j < (int32_t)m_resolution_cloth.y; ++j) {
				for (int32_t i = 0; i < (int32_t)m_resolution_cloth.x; ++i) {
					// create patch
					for (int32_t dj = -1; dj < 2; ++dj) {
						const int32_t new_j = std::clamp(j + dj, 0, (int32_t)m_resolution_cloth.y - 1);
						for (int32_t di = -1; di < 2; ++di) {
							const int32_t new_i = std::clamp(i + di, 0, (int32_t)m_resolution_cloth.x - 1);

							patch_indices.push_back(new_j * m_resolution_cloth.x + new_i);
						}
					}
				}
			}
			m_num_elements_patches = (uint32_t)patch_indices.size();

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patches_indices_bo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(patch_indices[0]) * patch_indices.size(),
				patch_indices.data(), GL_STATIC_DRAW);
			

			// Segment lengths
			std::vector<float> original_lengths(m_system_config.num_segments);
			for (uint32_t i = 0; i < m_system_config.num_segments; ++i) {
				original_lengths[i] = glm::length(p[indices[i].x].pos - p[indices[i].y].pos);
			}
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_original_lengths_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				sizeof(float) * original_lengths.size(),
				original_lengths.data(), GL_STATIC_DRAW);

			
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particle_2_segments_list);
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				sizeof(Particle2SegmentsList) * particle2segments_map.size(),
				particle2segments_map.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_segments_list_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				sizeof(SegmentMapping) * mappings.size(),
				mappings.data(), GL_STATIC_DRAW);
			// Upload also fixed particles, modify original points
			for (uint32_t i = 0; i < m_system_config.num_fixed_particles; ++i) {
				p[i].pos -= m_sphere_head.pos;
			}
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fixed_points_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				sizeof(Particle) * m_system_config.num_fixed_particles,
				p.data(),
				GL_STATIC_DRAW);
		}

		// force buffers
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_forces_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
			sizeof(glm::vec4) * m_system_config.num_segments,
			nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glClearNamedBufferSubData(m_forces_buffer, GL_R32F,
			0, sizeof(glm::vec4) * m_system_config.num_segments, GL_RED, GL_FLOAT, nullptr);

	}

	update_system_config();
	reset_bindings();
}

void ClothSystem::update_interaction_data()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere), sizeof(Sphere), &m_sphere_head);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	m_advect_particle_program.use_program();
	glUniform1ui(1, m_intersect_sphere ? 1 : 0);
	// Always disable second sphere
	glUniform1ui(2, 0);

	glUseProgram(0);

}

void ClothSystem::update_system_config()
{
	glNamedBufferSubData(
		m_system_config_bo, // buffer name
		0, // offset
		sizeof(SpringSystemConfig),	// size
		&m_system_config	// data
	);
}

void ClothSystem::set_sphere(const glm::vec3& pos, float radius)
{
	Sphere s;
	s.pos = pos;
	s.radius = radius * 1.01f; // make it slightly bigger for tessellation

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Sphere), &s);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ClothSystem::reset_bindings() const
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

