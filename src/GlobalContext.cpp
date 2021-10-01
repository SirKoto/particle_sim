#include "GlobalContext.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <array>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <particle_types.in>

GlobalContext::GlobalContext() {

    const std::filesystem::path shad_dir = std::filesystem::path(PROJECT_DIR) / "resources/shaders";

    std::array<Shader, 2> particle_shaders = { 
        Shader((shad_dir / "simpl.vert"), Shader::Type::Vertex ),
        Shader((shad_dir / "simpl.frag"), Shader::Type::Fragment)
    };

    m_particle_draw_program = ShaderProgram(particle_shaders.data(), (uint32_t)particle_shaders.size());
}

void GlobalContext::update()
{
    // update particle system from previous frame information
    // to use cpu time drawing the gui
    m_particle_sys.update();


    if (ImGui::Begin("Simulator")) {
        m_camera.renderImGui();

        m_particle_sys.imgui_draw();
    }
    ImGui::End();

    // Camera update
    m_camera.update();
}

void GlobalContext::render()
{
    m_particle_draw_program.use_program();
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.getProjView()));

    m_particle_sys.gl_render_particles();

    glUseProgram(0);
}
