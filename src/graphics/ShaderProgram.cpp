#include "ShaderProgram.hpp"
#include <glad/glad.h>
#include <iostream>

ShaderProgram::ShaderProgram(const Shader* shaders, uint32_t num)
{
	m_id = glCreateProgram();
	for (uint32_t i = 0; i < num; ++i) {
		glAttachShader(m_id, shaders[i].get_id());
	}

	glLinkProgram(m_id);

	int32_t success;
	glGetProgramiv(m_id, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetProgramInfoLog(m_id, 512, NULL, infoLog);
		std::cerr << "ERROR SHADER PROGRAM LINKING_FAILED\n" << infoLog << std::endl;
		exit(2);
	}
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& o)
{
	if (m_id != 0) {
		glDeleteProgram(m_id);
	}

	m_id = o.m_id;
	o.m_id = 0;

	return *this;
}

void ShaderProgram::use_program() const
{
	glUseProgram(m_id);
}

ShaderProgram::~ShaderProgram()
{
	if (m_id != 0) {
		glDeleteProgram(m_id);
	}
}
