#pragma once

#include <glm/glm.hpp>
#include "Camera.hpp"
#include "graphics/TriangleMesh.hpp"
#include "graphics/ShaderProgram.hpp"



class GlobalContext
{
public:

	GlobalContext();

	void update();

	void render();

	const glm::vec3& get_clear_color() const {
		return m_clear_color;
	}

private:
	glm::vec3 m_clear_color = glm::vec3(0.45f, 0.55f, 0.60f);
	Camera m_camera;

	TriangleMesh m_ico_mesh;
	ShaderProgram m_particle_draw_program;
	uint32_t m_ico_draw_vao;

};

