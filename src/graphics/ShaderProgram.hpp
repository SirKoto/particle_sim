#pragma once

#include "Shader.hpp"

class ShaderProgram {
public:
	ShaderProgram() = default;
	ShaderProgram(const Shader* shaders, uint32_t num);
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram& operator=(ShaderProgram&&);

	void use_program() const;

	~ShaderProgram();


private:
	uint32_t m_id = 0;
};