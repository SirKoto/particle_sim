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

m_floor_mesh = TriangleMesh({ {0, 1, 2}, {0, 2, 3} },
    { {0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f, 1.f}, {1.f, 0.f, 0.f} });
m_floor_mesh.upload_to_gpu();
std::array<Shader, 2> floor_shaders = {
    Shader((shad_dir / "floor.vert"), Shader::Type::Vertex),
    Shader((shad_dir / "floor.frag"), Shader::Type::Fragment)
};
m_floor_draw_program = ShaderProgram(floor_shaders.data(), (uint32_t)floor_shaders.size());
glGenVertexArrays(1, &m_floor_vao);
glBindVertexArray(m_floor_vao);
m_floor_mesh.gl_bind_to_vao();
glBindVertexArray(0);

// TODO: remove this
update_uniform_mesh();
m_particle_sys.set_mesh(m_mesh_mesh, get_mesh_transform());


m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);
m_cloth_sys.set_sphere(m_sphere_pos, m_sphere_radius);
m_spring_sys.set_sphere(m_sphere_pos, m_sphere_radius);

// Set default bindings 
switch (m_simulation_mode)
{
case SimulationMode::eParticle:
    m_particle_sys.reset_bindings();
    m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);

    break;
case SimulationMode::eSprings:
    m_spring_sys.reset_bindings();
    m_spring_sys.set_sphere(m_sphere_pos, m_sphere_radius);

    break;
case SimulationMode::eCloth:
    m_cloth_sys.reset_bindings();
    m_cloth_sys.set_sphere(m_sphere_pos, m_sphere_radius);
    break;
}
}

void GlobalContext::update()
{
    // update particle system from previous frame information
    // to use cpu time drawing the gui
    float time = (float)glfwGetTime();
    if (m_run_simulation) {

        float delta_time;
        switch (m_deltatime_mode)
        {
        case GlobalContext::DeltaTimeMode::eDynamic:
            delta_time = ImGui::GetIO().DeltaTime;
            break;
        case GlobalContext::DeltaTimeMode::eStaticMax:
            delta_time = 1.0f / (float)m_max_fps;
            break;
        default:
            break;
        }

        switch (m_simulation_mode)
        {
        case SimulationMode::eParticle:
            m_particle_sys.update(time, delta_time);
            break;
        case SimulationMode::eSprings:
            m_spring_sys.update(time, delta_time);
            break;
        case SimulationMode::eCloth:
            m_cloth_sys.update(time, delta_time);
            break;
        }
    }


    if (ImGui::BeginMainMenuBar())
    {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);

        if (ImGui::Combo("##combo_mode", (int32_t*)&m_simulation_mode, "Particles\0Springs\0Cloth")) {
            switch (m_simulation_mode)
            {
            case SimulationMode::eParticle:
                m_particle_sys.reset_bindings();
                m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);

                break;
            case SimulationMode::eSprings:
                m_spring_sys.reset_bindings();
                m_spring_sys.set_sphere(m_sphere_pos, m_sphere_radius);

                break;
            case SimulationMode::eCloth:
                m_cloth_sys.reset_bindings();
                m_cloth_sys.set_sphere(m_sphere_pos, m_sphere_radius);
                break;
            }
        }

        ImGui::Checkbox("Run Simulation", &m_run_simulation);

        if (ImGui::BeginMenu("View"))
        {
            if(ImGui::InputFloat("Line width", &m_line_width, 1.0f)) {
                glLineWidth(m_line_width);
            }

            ImGui::Checkbox("Scene info", &m_scene_window);
            ImGui::Checkbox("ImGui Demo Window", &m_show_imgui_demo_window);
            ImGui::Checkbox("Camera info", &m_show_camera_window);


            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::InputDouble("Max FPS", &m_max_fps, 1.0)) {
                m_max_fps = std::clamp(m_max_fps, 1.0, 500.0);
            }
            ImGui::Separator();
            ImGui::Combo("Time step mode", (int32_t*)&m_deltatime_mode, "Dynamic\0Static Max\0");

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
            ImGui::PushItemWidth(ImGui::GetFontSize() * -5);
            ImGui::PushID("Sphere");
            ImGui::Checkbox("Draw Sphere", &m_draw_sphere);

            bool update_sphere = ImGui::DragFloat3("Position", &m_sphere_pos.x, 0.01f);
            update_sphere |= ImGui::DragFloat("Radius", &m_sphere_radius, 0.01f);
                
            if (update_sphere) {
                switch (m_simulation_mode)
                {
                case SimulationMode::eParticle:
                    m_particle_sys.set_sphere(m_sphere_pos, m_sphere_radius);
                    break;
                case SimulationMode::eSprings:
                    m_spring_sys.set_sphere(m_sphere_pos, m_sphere_radius);
                    break;
                case SimulationMode::eCloth:
                    m_cloth_sys.set_sphere(m_sphere_pos, m_sphere_radius);
                    break;
                }
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
                switch (m_simulation_mode)
                {
                case SimulationMode::eParticle:
                    m_particle_sys.set_mesh(m_mesh_mesh, get_mesh_transform());
                    break;
                case SimulationMode::eSprings:
                    break;
                }
            }
            ImGui::PopID();
            ImGui::Separator();

            ImGui::Checkbox("Draw floor", &m_draw_floor);
            ImGui::PopItemWidth();

        }
        ImGui::End();
    }

    if (ImGui::Begin("Simulation Config")) {
        ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
        switch (m_simulation_mode)
        {
        case SimulationMode::eParticle:
            m_particle_sys.imgui_draw();
            break;
        case SimulationMode::eSprings:
            m_spring_sys.imgui_draw();
            break;
        case SimulationMode::eCloth:
            m_cloth_sys.imgui_draw();
            break;
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();


    // Camera update
    m_camera.update();

}

void GlobalContext::render()
{
    const glm::mat4 view_proj_mat = m_camera.getProjView();
    if (m_draw_sphere) {
        m_sphere_draw_program.use_program();
        glBindVertexArray(m_sphere_vao);
        glUniform3fv(0, 1, &m_sphere_pos.x);
        glUniform1f(1, m_sphere_radius);
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(view_proj_mat));
        glDrawElements(GL_TRIANGLES,
            3 * (GLsizei)m_sphere_mesh.get_faces().size(),
            GL_UNSIGNED_INT, (void*)0);
    }

    if (m_draw_mesh) {
        m_mesh_draw_program.use_program();
        glBindVertexArray(m_mesh_vao);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view_proj_mat));
        glDrawElements(GL_TRIANGLES,
            3 * (GLsizei)m_mesh_mesh.get_faces().size(),
            GL_UNSIGNED_INT, (void*)0);
    }

    if (m_draw_floor) {
        m_floor_draw_program.use_program();
        glBindVertexArray(m_floor_vao);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view_proj_mat));

        float sim_space_size;
        switch (m_simulation_mode)
        {
        case SimulationMode::eParticle:
            sim_space_size = m_particle_sys.get_simulation_space_size();
            break;
        case SimulationMode::eSprings:
            sim_space_size = m_spring_sys.get_simulation_space_size();
            break;
        case SimulationMode::eCloth:
            sim_space_size = m_cloth_sys.get_simulation_space_size();
            break;
        }


        glUniform1f(0, sim_space_size);
        glDrawElements(GL_TRIANGLES,
            3 * (GLsizei)m_floor_mesh.get_faces().size(),
            GL_UNSIGNED_INT, (void*)0);
    }

    switch (m_simulation_mode)
    {
    case SimulationMode::eParticle:
        m_particle_draw_program.use_program();
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view_proj_mat));

        m_particle_sys.gl_render_particles();
        break;
    case SimulationMode::eSprings:

        m_spring_sys.gl_render(view_proj_mat, m_camera.get_eye());
        break;
    case SimulationMode::eCloth:

        m_cloth_sys.gl_render(view_proj_mat, m_camera.get_eye());
        break;
    }

    
    
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
