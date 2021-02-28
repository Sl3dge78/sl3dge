#include "Camera.h"

#include "imgui/imgui.h"

#include "vulkan/VulkanApplication.h"

void Camera::start() {
	matrices.proj = glm::perspectiveRH(glm::radians(80.f), aspect_ratio, 0.001f, 1000.f);
	//matrices.proj[1][1] *= -1;
	matrices.proj_inverse = glm::inverse(matrices.proj);
	on_parent_changed();
}
void Camera::update(float delta_time) {
	if (matrices.frame <= max_samples) {
		matrices.frame++;
	}
	if (show_window) {
		display_window();
	}
}

void Camera::on_parent_changed() {
	update_xform();
	matrices.view = get_world_transform();
	matrices.view_inverse = glm::inverse(matrices.view);
	matrices.pos = get_world_position();
	matrices.frame = 0;
}

void Camera::display_window() {
	/*
	if (!ImGui::Begin("Camera", &show_window, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	}
	auto pos = get_world_position();
	ImGui::InputFloat3("Position", &pos[0]);
	ImGui::Separator();
	ImGui::InputFloat("yaw", &yaw, 0, 0, 3, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputFloat("pitch", &pitch, 0, 0, 3, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputInt("frame", &matrices.frame);
	ImGui::SliderInt("max samples", &max_samples, 0, 4096);
	ImGui::End();
	*/
}
