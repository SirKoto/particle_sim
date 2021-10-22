#include "Shader.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>


void fill_stream(const std::filesystem::path& path, std::stringstream& stream) {
	std::ifstream file;
	file.open(path);
	if (!file)
	{
		std::cerr << "ERROR SHADER FILE_NOT_SUCCESFULLY_READ: " << path << std::endl;
		exit(1);
	}
	else
	{
		std::string line;
		while (file) {
			std::getline(file, line);
			if (line.rfind("#include", 0) == 0) {
				size_t ini = line.find_first_of('\"');
				size_t last = line.find_last_of('\"');
				if (ini == last || ini + 1 >= last) {
					std::cerr << "Failed include, no \" pair in " << path << std::endl;
					continue;
				}
				std::string new_file = line.substr(ini + 1, last - ini - 1);
				fill_stream(path.parent_path() / new_file, stream);
			}
			else {
				stream << line << "\n";
			}

		}

		file.close();
	}
}

Shader::Shader(const std::filesystem::path& path, Type type)
	: m_type(type)
{
	
	std::stringstream stream;

	fill_stream(path, stream);
	
	const std::string code = stream.str();


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
		exit(1);
	}
}

Shader& Shader::operator=(Shader&& o)
{
	if (m_id != 0) {
		glDeleteShader(m_id);
	}

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
		return GL_COMPUTE_SHADER;
	case Type::TessellationControl:
		return GL_TESS_CONTROL_SHADER;
	case Type::TessellationEvaluation:
		return GL_TESS_EVALUATION_SHADER;
	default:
		assert(false);
	}
	return 0;
}

