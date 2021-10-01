#include "GlobalContext.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <array>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <particle_types.in>

GlobalContext::GlobalContext() :
    m_ico_mesh((std::filesystem::path(PROJECT_DIR) / "resources/ply/icosahedron.ply").string().c_str())
{
    m_ico_mesh.upload_to_gpu();

    const std::filesystem::path shad_dir = std::filesystem::path(PROJECT_DIR) / "resources/shaders";

    std::array<Shader, 2> particle_shaders = { 
        Shader((shad_dir / "simpl.vert"), Shader::Type::Vertex ),
        Shader((shad_dir / "simpl.frag"), Shader::Type::Fragment)
    };

    m_particle_draw_program = ShaderProgram(particle_shaders.data(), (uint32_t)particle_shaders.size());

    glGenVertexArrays(1, &m_ico_draw_vao);
    glBindVertexArray(m_ico_draw_vao);
    m_ico_mesh.gl_bind_to_vao();
    glBindVertexArray(0);
}

void GlobalContext::update()
{
    if (ImGui::Begin("Simulator")) {
        m_camera.renderImGui();
    }
    ImGui::End();

    // Camera update
    m_camera.update();
}

void GlobalContext::render()
{
    m_particle_draw_program.use_program();
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.getProjView()));

    glBindVertexArray(m_ico_draw_vao);

    glDrawElements(GL_TRIANGLES, (GLsizei)m_ico_mesh.get_faces().size() * 3, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    glUseProgram(0);
}
