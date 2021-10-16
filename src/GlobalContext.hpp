#pragma once

#include <glm/glm.hpp>
#include "Camera.hpp"
#include "graphics/TriangleMesh.hpp"
#include "graphics/ShaderProgram.hpp"
#include "particle_system/ParticleSystem.hpp"
#include "particle_system/SpringSystem.hpp"



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

	enum class SimulationMode {
		eParticle = 0,
		eSprings = 1,
	};

	SimulationMode m_simulation_mode = SimulationMode::eSprings;

	ShaderProgram m_particle_draw_program;
	ParticleSystem m_particle_sys;

	SpringSystem m_spring_sys;

	bool m_draw_sphere = true;
	uint32_t m_sphere_vao;
	ShaderProgram m_sphere_draw_program;
	TriangleMesh m_sphere_mesh;
	glm::vec3 m_sphere_pos = glm::vec3(5.0f, 0.0f, 5.0f); 
	float m_sphere_radius = 2.0f;

	bool m_draw_mesh = true;
	uint32_t m_mesh_vao;
	TriangleMesh m_mesh_mesh;
	ShaderProgram m_mesh_draw_program;
	glm::vec3 m_mesh_translation = glm::vec3(0.0f, 2.f, 5.0f);
	float m_mesh_scale = 2.0f;

	bool m_draw_floor = true;
	uint32_t m_floor_vao;
	TriangleMesh m_floor_mesh;
	ShaderProgram m_floor_draw_program;

	glm::mat4 get_mesh_transform() const;
	void update_uniform_mesh() const;
};

