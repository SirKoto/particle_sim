#pragma once

#include "graphics/ShaderProgram.hpp"
#include "graphics/TriangleMesh.hpp"


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
};