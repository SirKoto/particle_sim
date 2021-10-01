#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>


class TriangleMesh {
public:
	TriangleMesh(const char* path);
	~TriangleMesh();

	TriangleMesh(const TriangleMesh&) = delete;
	TriangleMesh& operator=(const TriangleMesh&) = delete;

	void print_debug_info() const;

	void write_mesh_ply(const char* fileName) const;

	void apply_transform(const glm::mat4& t);

	void upload_to_gpu(bool dynamic_verts= false, bool dynamic_indices = false);
	void gl_bind_to_vao() const;

	const std::vector<glm::vec3>& get_vertices() const {
		return m_vertices;
	}

	const std::vector<glm::uvec3>& get_faces() const {
		return m_faces;
	}

private:

	void parse_ply(const char* path);

	// Variables
	std::vector<glm::vec3> m_vertices;
	std::vector<glm::uvec3> m_faces;

	union
	{
		struct {
			uint32_t m_vbo_vertices;
			uint32_t m_vbo_indices;
		};
		uint32_t m_vbos[2];
	};
};

