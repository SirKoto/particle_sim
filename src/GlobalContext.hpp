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

	bool m_show_imgui_demo_window = false;
	bool m_show_camera_window = false;
	bool m_scene_window = false;

	bool m_run_simulation = true;

	ShaderProgram m_particle_draw_program;
	ParticleSystem m_particle_sys;

	bool m_draw_sphere = true;
	uint32_t m_sphere_vao;
	ShaderProgram m_sphere_draw_program;
	TriangleMesh m_sphere_mesh;
	glm::vec3 m_sphere_pos;
	float m_sphere_radius;

};

