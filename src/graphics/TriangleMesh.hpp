#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <filesystem>


class TriangleMesh {
public:
	TriangleMesh();
	TriangleMesh(const std::filesystem::path& path);
	~TriangleMesh();

	TriangleMesh(const TriangleMesh&);
	TriangleMesh& operator=(const TriangleMesh&) = delete;
	TriangleMesh& operator=(TriangleMesh&&);


	void print_debug_info() const;

	void write_mesh_ply(const char* fileName) const;

	void apply_transform(const glm::mat4& t);

	void upload_to_gpu(bool dynamic_verts= false, bool dynamic_indices = false);

	// Enables attrib 0 with vec3, vertex coordinates
	void gl_bind_to_vao() const;

	const std::vector<glm::vec3>& get_vertices() const {
		return m_vertices;
	}

	const std::vector<glm::uvec3>& get_faces() const {
		return m_faces;
	}

	uint32_t get_vbo_vertices() const { return m_vbo_vertices; }
	uint32_t get_vbo_indices() const { return m_vbo_indices; }


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

