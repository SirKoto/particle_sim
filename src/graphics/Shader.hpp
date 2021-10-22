#pragma once

#include <string>
#include <glm/glm.hpp>
#include <filesystem>


class Shader
{
public:
	enum class Type {
		Vertex,
		Fragment,
		Compute,
		TessellationControl,
		TessellationEvaluation,
	};

	Shader() = default;
	Shader(const std::filesystem::path& path, Type type);

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&&);


	~Shader();

	uint32_t get_gl_shader_type() const;

	uint32_t get_id() const { return m_id; }


private:
	uint32_t m_id = 0;
	Type m_type;
};
