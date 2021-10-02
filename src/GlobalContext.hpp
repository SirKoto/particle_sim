#pragma once

#include <glm/glm.hpp>
#include "Camera.hpp"
#include "graphics/TriangleMesh.hpp"
#include "graphics/ShaderProgram.hpp"
#include "particle_system/ParticleSystem.hpp"


class GlobalContext
{
public:

	GlobalContext();

	GlobalContext(const GlobalContext&) = delete;
	GlobalContext& operator=(const GlobalContext&) = delete;

	void update();

	void render();

	const glm::vec3& get_clear_color() const {
		return m_clear_color;
	}

private:
	glm::vec3 m_clear_color = glm::vec3(0.45f, 0.55f, 0.60f);
	Camera m_camera;

	bool m_show_imgui_demo_window = true;
	bool m_show_camera_window = false;

	ShaderProgram m_particle_draw_program;
	ParticleSystem m_particle_sys;

};

