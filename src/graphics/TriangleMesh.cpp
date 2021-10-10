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

TriangleMesh::TriangleMesh(
	const std::vector<glm::uvec3>& indices,
	const std::vector<glm::vec3>& vertices)
{
	glGenBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	m_faces = indices;
	m_vertices = vertices;
	
	generate_normals();
}

TriangleMesh::~TriangleMesh()
{
	if (m_vbo_vertices != 0) {
		glDeleteBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	}
}

TriangleMesh::TriangleMesh(const TriangleMesh& o)
{
	glGenBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	m_vertices = o.m_vertices;
	m_faces = o.m_faces;
	m_normals = o.m_normals;
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
	m_normals = std::move(o.m_normals);

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

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);
	glBufferData(GL_ARRAY_BUFFER,
		m_normals.size() * sizeof(glm::vec3),
		m_normals.data(),
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

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(1);

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
	m_normals.resize(vertices->count);
	for (size_t i = 0; i < vertices->count; ++i) {
		std::memcpy(&m_vertices[i], vertices->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		//if (texcoords) {
		//	std::memcpy(&(*outVertices)[i].texCoord, texcoords->buffer.get() + i * 2 * sizeof(float), 2 * sizeof(float));
		//}
		if (normals) {
			std::memcpy(&m_normals[i], normals->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		}
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

	if (!normals) {
		generate_normals();
	}
}

void TriangleMesh::generate_normals()
{
	m_normals.clear();
	m_normals.resize(m_vertices.size());

	// Compute the planes of all triangles
	std::vector<glm::vec3> triangleNormals(m_faces.size());
	for (uint32_t t = 0; t < (uint32_t)m_faces.size(); ++t) {
		const glm::vec3& v0 = m_vertices[m_faces[t][0]];
		const glm::vec3& v1 = m_vertices[m_faces[t][1]];
		const glm::vec3& v2 = m_vertices[m_faces[t][2]];

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
		triangleNormals[t] = glm::normalize(n);
	}

	// Compute V:{F}
	std::vector<std::vector<uint32_t>> vert2faces(m_vertices.size());
	std::vector<uint32_t> vertexArity(m_vertices.size(), 0);
	for (uint32_t t = 0; t < (uint32_t)m_faces.size(); ++t) {
		vertexArity[m_faces[t][0]] += 1;
		vertexArity[m_faces[t][1]] += 1;
		vertexArity[m_faces[t][2]] += 1;
	}
	for (uint32_t v = 0; v < (uint32_t)m_vertices.size(); ++v) {
		vert2faces[v].reserve(vertexArity[v]);
	}
	for (uint32_t t = 0; t < (uint32_t)m_faces.size(); ++t) {
		vert2faces[m_faces[t][0]].push_back(t);
		vert2faces[m_faces[t][1]].push_back(t);
		vert2faces[m_faces[t][2]].push_back(t);
	}

	for (uint32_t v = 0; v < (uint32_t)m_normals.size(); ++v) {
		m_normals[v] = glm::vec3(0);

		for (const uint32_t& f : vert2faces[v]) {
			m_normals[v] += triangleNormals[f];
		}
		m_normals[v] /= vert2faces[v].size();
	}
}
