#include "SpringSystem.hpp"

#include <imgui.h>


SpringSystem::SpringSystem()
{
}

void SpringSystem::update(float time, float dt)
{
}

void SpringSystem::gl_render_triangle_ribbons()
{
}

void SpringSystem::imgui_draw()
{
	ImGui::PushID("springSys");
	ImGui::Text("Spring System Config");

	ImGui::PopID();
}

void SpringSystem::reset_bindings() const
{
}
