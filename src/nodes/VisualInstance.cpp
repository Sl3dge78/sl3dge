#include "nodes/VisualInstance.h"

#include <imgui/imgui.h>

uint32_t VisualInstance::next_instance_id = 0;

void VisualInstance::draw_gui() {
	Node3D::draw_gui();
	ImGui::Separator();
	ImGui::Text("Visual Instance");
}
