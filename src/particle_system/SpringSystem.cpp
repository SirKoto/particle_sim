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

	// Generate sphere
	m_sphere_mesh = TriangleMesh(proj_dir / "resources/ply/sphere.ply");
	m_sphere_mesh.upload_to_gpu();
	std::array<Shader, 2> sphere_shaders = {
		Shader((shad_dir / "sphere.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "sphere.frag"), Shader::Type::Fragment)
	};
	m_sphere_draw_program = ShaderProgram(sphere_shaders.data(), (uint32_t)sphere_shaders.size());

	std::array<Shader, 4> hair_shaders = {
		Shader((shad_dir / "hair.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "hair.frag"), Shader::Type::Fragment),
		Shader((shad_dir / "hair.tesc"), Shader::Type::TessellationControl),
		Shader((shad_dir / "hair.tese"), Shader::Type::TessellationEvaluation)
	};
	m_hair_draw_program = ShaderProgram(hair_shaders.data(), (uint32_t)hair_shaders.size());

	glGenVertexArrays(1, &m_sphere_vao);
	glBindVertexArray(m_sphere_vao);
	m_sphere_mesh.gl_bind_to_vao();
	glBindVertexArray(0);

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
	glUniform4fv(3, 1, glm::value_ptr(m_rotation));
	glDispatchCompute(m_system_config.num_particles / 32
		+ (m_system_config.num_particles % 32 == 0 ? 0 : 1)
		, 1, 1);

	// flip state
	m_flipflop_state = !m_flipflop_state;
}

void SpringSystem::gl_render(const glm::mat4& proj_view, const glm::vec3& eye_world)
{

	// Draw sphere
	if (m_init_system == InitSystems::eSphere && m_draw_head) {
		m_sphere_draw_program.use_program();
		glBindVertexArray(m_sphere_vao);
		glUniform3fv(0, 1, glm::value_ptr(m_sphere_head.pos));
		glUniform1f(1, m_sphere_head.radius);
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_view));
		glDrawElements(GL_TRIANGLES,
			3 * (GLsizei)m_sphere_mesh.get_faces().size(),
			GL_UNSIGNED_INT, (void*)0);
	}

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

		m_hair_draw_program.use_program();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(proj_view));
		glUniform3fv(2, 1, glm::value_ptr(eye_world));
		glUniform1f(3, m_hair_specular_alpha);
		glUniform3fv(4, 1, glm::value_ptr(m_hair_specular));
		glUniform3fv(5, 1, glm::value_ptr(m_hair_diffuse));

		glPatchParameteri(GL_PATCH_VERTICES, 3);
		
		//glDrawArrays(GL_PATCHES, 0, 3);
		glBindVertexArray(m_patches_vao);
		glDrawElements(GL_PATCHES,
			m_num_elements_patches,
			GL_UNSIGNED_INT, nullptr);
		
		if (m_draw_points) {
			m_basic_draw_point.use_program();
			glBindVertexArray(m_segment_vao);
			glPointSize(10.0f);
			glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(proj_view));
			glDrawArrays(GL_POINTS, 0, m_system_config.num_particles);
		}

	}

	glBindVertexArray(0);
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
	update |= ImGui::InputFloat("Friction", &m_system_config.friction, 0.01f);

	update |= ImGui::DragFloat("K elastic", &m_system_config.k_e, 0.1f);
	update |= ImGui::DragFloat("K damping", &m_system_config.k_d, 0.1f);
	update |= ImGui::DragFloat("Particle mass", &m_system_config.particle_mass, 0.1f);

	if (update) {
		update_system_config();
	}

	ImGui::Text("Interaction:");
	if (ImGui::DragFloat3("Position", glm::value_ptr(m_sphere_head.pos), 0.01f)) {
		update_interaction_data();
	}

	ImGui::InputFloat3("Rotation axis", glm::value_ptr(m_rotation_axis));
	ImGui::SameLine();
	if (ImGui::Button("Normalize")) {
		m_rotation_axis = glm::normalize(m_rotation_axis);
	}

	float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	ImGui::PushButtonRepeat(true);
	if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
		const float s = std::sinf(-m_rotation_rate);
		const float c = std::cosf(-m_rotation_rate);
		glm::quat q(c, s * m_rotation_axis);

		m_rotation = q * m_rotation;
	}
	ImGui::SameLine(0.0f, spacing);
	if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { 
		const float s = std::sinf(m_rotation_rate);
		const float c = std::cosf(m_rotation_rate);
		glm::quat q(c, s * m_rotation_axis);

		m_rotation = q * m_rotation;
	}
	ImGui::PopButtonRepeat();
	
	ImGui::SameLine();
	ImGui::InputFloat("Rate", &m_rotation_rate);

	ImGui::Separator();
	if(ImGui::Checkbox("Sphere collisions", &m_intersect_sphere)) {
		update_intersection_sphere();
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

		ImGui::ColorEdit3("Diffuse color", glm::value_ptr(m_hair_diffuse));
		ImGui::ColorEdit3("Specular color", glm::value_ptr(m_hair_specular));
		ImGui::InputFloat("Alpha specular", &m_hair_specular_alpha, 1.0f);

		ImGui::Checkbox("Draw Points", &m_draw_points);

		ImGui::PopID();
	}
	if (m_init_system == InitSystems::eSphere) {
		ImGui::Checkbox("Draw head sphere", &m_draw_head);
	}


	ImGui::Separator();

	if (ImGui::Combo("Init system", reinterpret_cast<int*>(&m_init_system), "Rope\0Sphere\0")) {
		m_head_sphere_enabled = m_init_system == InitSystems::eSphere;
		update_interaction_data();
	}

	if (m_init_system == InitSystems::eRope) {
		ImGui::PushID("RopeInit");
		ImGui::DragFloat3("Rope direction", glm::value_ptr(m_rope_init_dir), 0.01f);
		ImGui::InputScalar("Num particles", ImGuiDataType_U32, &m_rope_init_num_particles);
		ImGui::InputFloat("Rope length", &m_rope_init_length, 0.1f);
		ImGui::InputScalar("Num fixed particles", ImGuiDataType_U32, &m_rope_init_num_fixed_particles);

		ImGui::PopID();
	}
	else if (m_init_system == InitSystems::eSphere) {
		ImGui::PushID("SphereInit");
		//ImGui::DragFloat("Radius", &m_sphere_head.radius, 0.01f, 0.0f, FLT_MAX);
		ImGui::InputScalar("Num hairs", ImGuiDataType_U32, &m_sphere_init_num_hairs);
		ImGui::InputScalar("Particles per strand", ImGuiDataType_U32, &m_sphere_init_particles_per_strand);
		ImGui::InputFloat("Hair length", &m_hair_length, 0.1f);

		ImGui::PopID();
	}
	else {
		assert(false);
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
	case InitSystems::eSphere:
		init_system_sphere();
		break;
	default:
		assert(false);
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

	update_system_config();
	reset_bindings();
}

void SpringSystem::update_system_config()
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
	m_system_config.num_particles_per_strand = num_particles;

	std::vector<Particle> p(num_particles);
	const glm::vec3 dir = glm::normalize(m_rope_init_dir);
	float delta_x = m_rope_init_length / (float)m_rope_init_num_particles;
	for (uint32_t i = 0; i < num_particles; ++i) {
		p[i].pos = m_sphere_head.pos + dir * ((float)i * delta_x);
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

	// Patch indices
	std::vector<glm::ivec3> patch_indices; patch_indices.reserve(m_system_config.num_particles);
	patch_indices.push_back({0, 0, 1 });
	for (uint32_t i = 0; i < m_system_config.num_particles - 2; i += 1) {
		patch_indices.push_back({ i, i + 1, i + 2 });
	}
	patch_indices.push_back({ m_system_config.num_particles - 2, m_system_config.num_particles - 1 , m_system_config.num_particles - 1 });

	m_num_elements_patches =  3 * (uint32_t)patch_indices.size();

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
		p[i].pos -= m_sphere_head.pos;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fixed_points_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Particle) * m_rope_init_num_fixed_particles,
		p.data(),
		GL_STATIC_DRAW);

}

// Generation of points at the same distance on the unit sphere
// extracted from https://bduvenhage.me/geometry/2019/07/31/generating-equidistant-vectors.html
void fibonacci_spiral_sphere(std::vector<Particle>* vectors_, const int32_t num_points) {
	std::vector<Particle>& vectors = *vectors_;
	vectors.reserve(num_points);

	const double gr = (std::sqrt(5.0) + 1.0) / 2.0;  // golden ratio = 1.6180339887498948482
	const double ga = (2.0 - gr) * (2.0 * glm::pi<double>());  // golden angle = 2.39996322972865332

	for (size_t i = 1; i <= num_points; ++i) {
		const double lat = std::asin(-1.0 + 2.0 * double(i) / (num_points + 1));
		const double lon = ga * i;

		const float x = static_cast<float>(std::cos(lon) * std::cos(lat));
		const float y = static_cast<float>(std::sin(lon) * std::cos(lat));
		const float z = static_cast<float>(std::sin(lat));

		vectors.emplace_back(Particle{ glm::vec3{ x, y, z }, 0.0f });
		//float len = std::sqrt(x*x + y*y + z*z);
		//std::cout << len << std::endl;

		//std::cout<< "("<<x<<","<<y<<","<<z<<"),";
	}
	//std::cout << std::endl;
}

void SpringSystem::init_system_sphere()
{
	m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	const uint32_t num_particles = m_system_config.num_particles = m_sphere_init_num_hairs * m_sphere_init_particles_per_strand;
	m_system_config.num_segments = m_sphere_init_num_hairs * (m_sphere_init_particles_per_strand - 1);
	m_system_config.num_particles_per_strand = m_sphere_init_particles_per_strand;

	std::vector<Particle> particles;
	particles.reserve(num_particles);
	std::vector<glm::ivec2> indices; indices.reserve(m_system_config.num_segments);

	// fill starting points
	fibonacci_spiral_sphere(&particles, m_sphere_init_num_hairs);
	// Upload fixed particles
	m_system_config.num_fixed_particles = m_sphere_init_num_hairs;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fixed_points_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Particle) * m_sphere_init_num_hairs,
		particles.data(),
		GL_STATIC_DRAW);

	std::vector<SegmentMapping> mappings;
	mappings.reserve(m_sphere_init_num_hairs * (m_sphere_init_particles_per_strand * 2 - 2));
	std::vector<Particle2SegmentsList> particle2segments_map(num_particles);

	std::vector<glm::ivec3> patch_indices; patch_indices.reserve(m_system_config.num_particles);

	// Create strands
	float delta_x = m_hair_length / (m_sphere_head.radius * (float)m_sphere_init_particles_per_strand);
	for (uint32_t root = 0; root < m_sphere_init_num_hairs; ++root) {
		// mappings of root (maybe are not needed...)
		mappings.push_back({ (uint32_t)indices.size(), 0 });
		particle2segments_map[root].segment_mapping_idx =  (uint32_t)mappings.size() - 1;
		particle2segments_map[root].num_segments = 1;

		// Add patch
		if(m_sphere_init_particles_per_strand > 1){
			// Add first segment
			const uint32_t start = (int32_t)particles.size();
			patch_indices.push_back({ root, root, start });
			for (uint32_t i = 0; i < m_sphere_init_particles_per_strand - 2; ++i) {
				const uint32_t idx = start + i;
				if (i == 0) {
					patch_indices.push_back({ root, idx, idx + 1});
				}
				else {
					patch_indices.push_back({ idx -1, idx, idx + 1 });
				}

				if (i == m_sphere_init_particles_per_strand - 3) {
					patch_indices.push_back({ idx, idx + 1, idx + 1 });
				}
			}
		}


		const glm::vec3 dir = particles[root].pos; // it is already normalized
		for (uint32_t i = 1; i < m_sphere_init_particles_per_strand; ++i) {

			const uint32_t particle_idx = (uint32_t)particles.size();
			particles.push_back(
				{
					dir + dir * delta_x * (float)i,
					0.0f
				}
			);

			if (i == 1) {
				indices.push_back(glm::ivec2(root, particle_idx));
			}
			else {
				indices.push_back(glm::ivec2(particle_idx - 1, particle_idx));
			}

			// Always add previous segment
			uint32_t num = 1;
			uint32_t idx = (uint32_t)mappings.size();
			mappings.push_back({ (uint32_t)indices.size() - 1, 1 });

			if (i != m_sphere_init_particles_per_strand - 1) {
				mappings.push_back({ (uint32_t)indices.size(), 0 });
				num += 1;
			}

			particle2segments_map[particle_idx].segment_mapping_idx = idx;
			particle2segments_map[particle_idx].num_segments = num;
		}
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particle_2_segments_list);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(Particle2SegmentsList) * particle2segments_map.size(),
		particle2segments_map.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_segments_list_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(SegmentMapping) * mappings.size(),
		mappings.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spring_indices_bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(glm::ivec2) * indices.size(),
		indices.data(), GL_STATIC_DRAW);

	m_num_elements_patches = 3 * (uint32_t)patch_indices.size();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patches_indices_bo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(patch_indices[0]) * patch_indices.size(),
		patch_indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Segment lengths
	std::vector<float> original_lengths(m_system_config.num_segments);
	for (uint32_t i = 0; i < m_system_config.num_segments; ++i) {
		original_lengths[i] = delta_x;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_original_lengths_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(float) * original_lengths.size(),
		original_lengths.data(), GL_STATIC_DRAW);


	// Update all particle positions before upload
	for (uint32_t i = 0; i < num_particles; ++i) {
		particles[i].pos = particles[i].pos * m_sphere_head.radius + m_sphere_head.pos;
	}
	glNamedBufferData(
		m_vbo_particle_buffers[0], // buffer name
		num_particles * sizeof(Particle),	// size
		particles.data(),	// data
		GL_DYNAMIC_DRAW
	);
	glNamedBufferData(
		m_vbo_particle_buffers[1], // buffer name
		num_particles * sizeof(Particle),	// size
		particles.data(),	// data
		GL_DYNAMIC_DRAW
	);

}

void SpringSystem::update_interaction_data()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphere_ssb);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere), sizeof(Sphere), &m_sphere_head);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	m_advect_particle_program.use_program();
	glUniform1ui(2, m_head_sphere_enabled ? 1 : 0);
}