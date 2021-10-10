#include "GlobalContext.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <array>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#include <particle_types.in>

GlobalContext::GlobalContext() {

    const std::filesystem::path proj_dir(PROJECT_DIR);
    const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

    std::array<Shader, 2> particle_shaders = { 
        Shader((shad_dir / "simpl.vert"), Shader::Type::Vertex ),
        Shader((shad_dir / "simpl.frag"), Shader::Type::Fragment)
    };

    m_particle_draw_program = ShaderProgram(particle_shaders.data(), (uint32_t)particle_shaders.size());

    m_sphere_mesh = TriangleMesh(proj_dir / "resources/ply/sphere.ply");
    m_sphere_mesh.upload_to_gpu();
    std::array<Shader, 2> sphere_shaders = {
        Shader((shad_dir / "sphere.vert"), Shader::Type::Vertex),
        Shader((shad_dir / "sphere.frag"), Shader::Type::Fragment)
    };
    m_sphere_draw_program = ShaderProgram(sphere_shaders.data(), (uint32_t)sphere_shaders.size());
    
    glGenVertexArrays(1, &m_sphere_vao);
    glBindVertexArray(m_sphere_vao);
    m_sphere_mesh.gl_bind_to_vao();
    glBindVertexArray(0);

    std::array<Shader, 2> mesh_shaders = {
        Shader((shad_dir / "mesh.vert"), Shader::Type::Vertex),
        Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
    };
    m_mesh_mesh = TriangleMesh(proj_dir / "resources/ply/icosahedron.ply");
    m_mesh_mesh.upload_to_gpu();
    m_mesh_draw_program = ShaderProgram(mesh_shaders.data(), (uint32_t)mesh_shaders.size());
    glGenVertexArrays(1, &m_mesh_vao);
    glBindVertexArray(m_mesh_vao);
    m_mesh_mesh.gl_bind_to_vao();
    glBindVertexArray(0);


    // TODO: remove this
    update_uniform_mesh();
    m_particle_sys.set_mesh(m_mesh_mesh, get_mesh_transform());


    m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);
}

void GlobalContext::update()
{
    // update particle system from previous frame information
    // to use cpu time drawing the gui
    float time = (float)glfwGetTime();
    if (m_run_simulation) {
        m_particle_sys.update(time, ImGui::GetIO().DeltaTime);
    }


    if (ImGui::BeginMainMenuBar())
    {
        ImGui::Checkbox("Run Simulation", &m_run_simulation);

        if (ImGui::BeginMenu("View"))
        {
            ImGui::Checkbox("Scene info", &m_scene_window);
            ImGui::Checkbox("ImGui Demo Window", &m_show_imgui_demo_window);
            ImGui::Checkbox("Camera info", &m_show_camera_window);


            ImGui::EndMenu();
        }

        ImGui::Text("Framerate %.1f", ImGui::GetIO().Framerate);
        ImGui::EndMainMenuBar();
    }


    if (m_show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
    }

    if (m_show_camera_window) {
        if (ImGui::Begin("Camera", &m_show_camera_window)) {
            m_camera.renderImGui();
        }
        ImGui::End();
    }

    if (m_scene_window) {
        ImGui::SetNextWindowSize(ImVec2(250, 180), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Scene", &m_scene_window)) {
            ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
            ImGui::PushID("Sphere");
            ImGui::Checkbox("Draw Sphere", &m_draw_sphere);
            if (ImGui::DragFloat3("Position", &m_sphere_pos.x, 0.01f)) {
                m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);
            }
            if (ImGui::DragFloat("Radius", &m_sphere_radius, 0.01f)) {
                m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);
            }
            ImGui::PopID();
            ImGui::Separator();
            ImGui::PushID("Mesh");
            ImGui::Checkbox("Draw Mesh", &m_draw_mesh);
            if (ImGui::DragFloat3("Position", &m_mesh_translation.x, 0.01f)) {
                update_uniform_mesh();
            }
            if (ImGui::DragFloat("Scale", &m_mesh_scale, 0.01f)) {
                update_uniform_mesh();
            }
            if (ImGui::Button("Send to simulator")) {
                m_particle_sys.set_mesh(m_mesh_mesh, get_mesh_transform());
            }

            ImGui::PopID();
            ImGui::PopItemWidth();
        }
        ImGui::End();
    }

    if (ImGui::Begin("Simulation Config")) {
        ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
        m_particle_sys.imgui_draw();
        ImGui::PopItemWidth();
    }
    ImGui::End();


    // Camera update
    m_camera.update();

}

void GlobalContext::render()
{
    if (m_draw_sphere) {
        m_sphere_draw_program.use_program();
        glBindVertexArray(m_sphere_vao);
        glUniform3fv(0, 1, &m_sphere_pos.x);
        glUniform1f(1, m_sphere_radius);
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.getProjView()));
        glDrawElements(GL_TRIANGLES,
            3 * (GLsizei)m_sphere_mesh.get_faces().size(),
            GL_UNSIGNED_INT, (void*)0);
    }

    if (m_draw_mesh) {
        m_mesh_draw_program.use_program();
        glBindVertexArray(m_mesh_vao);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.getProjView()));
        glDrawElements(GL_TRIANGLES,
            3 * (GLsizei)m_mesh_mesh.get_faces().size(),
            GL_UNSIGNED_INT, (void*)0);
    }


    m_particle_draw_program.use_program();
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.getProjView()));

    m_particle_sys.gl_render_particles();
    
    glUseProgram(0);
    assert(glGetError() == GL_NO_ERROR);
}

glm::mat4 GlobalContext::get_mesh_transform() const
{
    glm::mat4 t(1.0f);
    t = glm::translate(t, m_mesh_translation);
    t = glm::scale(t, glm::vec3(m_mesh_scale));
    return t;
}

void GlobalContext::update_uniform_mesh() const
{
    m_mesh_draw_program.use_program();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(get_mesh_transform()));
}
