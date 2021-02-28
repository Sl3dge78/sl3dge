#include "MeshInstance.h"
#include <imgui/imgui.h>

void MeshInstance::draw(vk::CommandBuffer cmd) {
	mesh->bind(cmd);

	cmd.drawIndexed(mesh->get_index_count(), 1, 0, 0, get_instance_id());
}

void MeshInstance::draw_gui() {
	VisualInstance::draw_gui();
	ImGui::Separator();
	ImGui::Text("Mesh Instance");
}
