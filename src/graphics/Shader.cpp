#include "Shader.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>


Shader::Shader(const char* path, Type type)
	: m_type(type)
{
	std::string code;
	std::ifstream file;
	
	file.open(path);
	if (!file)
	{
		std::cerr << "ERROR SHADER FILE_NOT_SUCCESFULLY_READ: " << path << std::endl;
	}
	else
	{
		std::stringstream stream;

		stream << file.rdbuf();

		file.close();
		code = stream.str();
	}


	// compilation
	GLint success;
	const char* code_cstr = code.c_str();

	m_id = glCreateShader(this->get_gl_shader_type());


	glShaderSource(m_id, 1, &code_cstr, NULL);
	glCompileShader(m_id);
	//errors?
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(m_id, 512, NULL, infoLog);
		std::cerr << "ERROR SHADER COMPILATION_FAILED "<< path << "\n" << infoLog << std::endl;
	}
}

Shader& Shader::operator=(Shader&& o)
{
	this->~Shader();

	m_id = o.m_id;
	o.m_id = 0;

	return *this;
}

Shader::~Shader() {
	if (m_id != 0) {
		glDeleteShader(m_id);
	}
}

uint32_t Shader::get_gl_shader_type() const
{
	switch (m_type)
	{
	case Type::Vertex:
		return GL_VERTEX_SHADER;
	case Type::Fragment:
		return GL_FRAGMENT_SHADER;
	case Type::Compute:
	default:
		return GL_COMPUTE_SHADER;
	}
}

