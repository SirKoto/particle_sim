#include "ParticleSystem.hpp"

#include <array>
#include <glad/glad.h>
#include <imgui.h>

ParticleSystem::ParticleSystem()
{
	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	m_ico_mesh = TriangleMesh(proj_dir / "resources/ply/icosahedron.ply");
	m_ico_mesh.upload_to_gpu();


	glGenVertexArrays(1, &m_ico_draw_vao);
	glBindVertexArray(m_ico_draw_vao);
	m_ico_mesh.gl_bind_to_vao();
	glBindVertexArray(0);
}

void ParticleSystem::update()
{
}

void ParticleSystem::gl_render_particles() const
{
	glBindVertexArray(m_ico_draw_vao);

	glDrawElements(GL_TRIANGLES, (GLsizei)m_ico_mesh.get_faces().size() * 3, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
}

void ParticleSystem::imgui_draw()
{
	ImGui::Text("Particle System Config");
	ImGui::Separator();
}
