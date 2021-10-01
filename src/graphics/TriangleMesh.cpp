#include "TriangleMesh.hpp"
#include <fstream>
#include <iostream>
#include <tinyply.h>
#include <glad/glad.h>

TriangleMesh::TriangleMesh()
{
	std::memset(m_vbos, 0, sizeof(m_vbos));
}

TriangleMesh::TriangleMesh(const std::filesystem::path& path)
{
	parse_ply(path.string().c_str());

	glGenBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
}

TriangleMesh::~TriangleMesh()
{
	glDeleteBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
}

TriangleMesh& TriangleMesh::operator=(TriangleMesh&& o)
{
	if (m_vbo_vertices != 0) {
		glDeleteBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	}
	
	std::memcpy(m_vbos, o.m_vbos, sizeof(m_vbos));
	std::memset(o.m_vbos, 0, sizeof(m_vbos));

	m_vertices = std::move(o.m_vertices);
	m_faces = std::move(o.m_faces);

	return *this;
}

void TriangleMesh::print_debug_info() const
{
	std::cout << "Mesh with:\n"
		"\tNum Vertices: " << m_vertices.size() << "\n"
		"\tNum Faces     " << m_faces.size() << std::endl;
}

void TriangleMesh::write_mesh_ply(const char* fileName) const
{
	std::ofstream stream(fileName, std::ios::binary | std::ios::trunc);

	tinyply::PlyFile file;

	file.add_properties_to_element("vertex", { "x", "y", "z" },
		tinyply::Type::FLOAT32, m_vertices.size(),
		reinterpret_cast<const uint8_t*>(m_vertices.data()),
		tinyply::Type::INVALID, 0);

	file.add_properties_to_element("face", { "vertex_indices" },
		tinyply::Type::INT32, m_faces.size(),
		reinterpret_cast<const uint8_t*>(m_faces.data()),
		tinyply::Type::UINT8, 3);

	file.write(stream, true);
}

void TriangleMesh::apply_transform(const glm::mat4& t)
{
	for (glm::vec3& v : m_vertices) {
		v = glm::vec3(t * glm::vec4(v, 1.0));
	}
}

void TriangleMesh::upload_to_gpu(bool dynamic_verts, bool dynamic_indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER,
		m_vertices.size() * sizeof(glm::vec3),
		m_vertices.data(),
		dynamic_verts ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		m_faces.size() * sizeof(glm::uvec3),
		m_faces.data(),
		dynamic_indices ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleMesh::gl_bind_to_vao() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);

}


void TriangleMesh::parse_ply(const char* fileName)
{
	std::ifstream stream(fileName, std::ios::binary);

	if (!stream) {
		throw std::runtime_error("Error: Can't open file " + std::string(fileName));
	}

	tinyply::PlyFile file;
	bool res = file.parse_header(stream);
	if (!res) {
		throw std::runtime_error("Error: Can't parse ply header.");
	}

	bool recomputeNormals = false;

	std::shared_ptr<tinyply::PlyData> vertices, normals, texcoords, faces;
	try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
	catch (const std::exception&) {}

	try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
	catch (const std::exception&) { recomputeNormals = true; }

	try { texcoords = file.request_properties_from_element("vertex", { "u", "v" }); }
	catch (const std::exception&) {}
	try { texcoords = file.request_properties_from_element("vertex", { "s", "t" }); }
	catch (const std::exception&) {}

	try { faces = file.request_properties_from_element("face", { "vertex_indices" }, 3); }
	catch (const std::exception&) {}

	file.read(stream);

	if (!vertices || !faces) {
		throw std::runtime_error("Error: Can't load faces of ply.");
	}

	assert(vertices->t == tinyply::Type::FLOAT32);
	assert(!normals || normals->t == tinyply::Type::FLOAT32);
	assert(!texcoords || texcoords->t == tinyply::Type::FLOAT32);

	// copy vertices
	m_vertices.resize(vertices->count);
	for (size_t i = 0; i < vertices->count; ++i) {
		std::memcpy(&m_vertices[i], vertices->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		//if (texcoords) {
		//	std::memcpy(&(*outVertices)[i].texCoord, texcoords->buffer.get() + i * 2 * sizeof(float), 2 * sizeof(float));
		//}
		//if (normals) {
		//	std::memcpy(&(*outVertices)[i].normal, normals->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		//}
	}

	m_faces.resize(faces->count);
	if (faces->t == tinyply::Type::UINT32 || faces->t == tinyply::Type::INT32) {
		std::memcpy(m_faces.data(), faces->buffer.get(), faces->buffer.size_bytes());
	}
	else if (faces->t == tinyply::Type::UINT16 || faces->t == tinyply::Type::INT16) {
		for (size_t i = 0; i < faces->count; ++i) {
			int16_t tmp[3];
			std::memcpy(tmp, faces->buffer.get() + i * 3 * sizeof(int16_t), 3 * sizeof(uint16_t));
			m_faces[i].x = static_cast<int32_t>(tmp[0]);
			m_faces[i].y = static_cast<int32_t>(tmp[1]);
			m_faces[i].z = static_cast<int32_t>(tmp[2]);
		}
	}
	else {
		throw std::runtime_error("Error: Cant read face format");
	}
}