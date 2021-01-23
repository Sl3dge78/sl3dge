#include "Camera.h"

#include "imgui/imgui.h"

#include "vulkan/VulkanApplication.h"

void Camera::load(VulkanApplication *app) {
	buffer = std::unique_ptr<Buffer>(new Buffer(*app, sizeof(Matrices), vk::BufferUsageFlagBits::eUniformBuffer, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible }));
}
void Camera::start() {
	position = glm::vec3(1.5f, 1.5f, 1.f);

	yaw = 220.f;
	pitch = -8.f;

	matrices.proj = glm::perspective(glm::radians(80.f), aspect_ratio, 0.001f, 1000.f);
	matrices.proj[1][1] *= -1;
	matrices.proj_inverse = glm::inverse(matrices.proj);

	update_vectors();
	buffer->write_data(&matrices, sizeof(Matrices));
}
void Camera::update(float delta_time) {
	if (Input::get_mouse(3)) {
		if (Input::delta_mouse_x() != 0 || Input::delta_mouse_y() != 0) {
			is_dirty = true;
		}
		yaw += Input::delta_mouse_x() * camera_speed * delta_time;
		pitch -= Input::delta_mouse_y() * camera_speed * delta_time;

		if (yaw > 360)
			yaw = 0;
		if (yaw < 0)
			yaw = 360;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	float delta_speed = delta_time * move_speed;

	if (Input::get_key(SDL_SCANCODE_LSHIFT))
		delta_speed *= 2.0f;

	if (Input::get_key(SDL_SCANCODE_W)) {
		position += delta_speed * front;
		is_dirty = true;
	}
	if (Input::get_key(SDL_SCANCODE_S)) {
		position -= delta_speed * front;
		is_dirty = true;
	}
	if (Input::get_key(SDL_SCANCODE_A)) {
		position -= delta_speed * glm::normalize(glm::cross(front, up));
		is_dirty = true;
	}
	if (Input::get_key(SDL_SCANCODE_D)) {
		position += delta_speed * glm::normalize(glm::cross(front, up));
		is_dirty = true;
	}
	if (Input::get_key(SDL_SCANCODE_Q)) {
		position -= delta_speed * up;
		is_dirty = true;
	}
	if (Input::get_key(SDL_SCANCODE_E)) {
		position += delta_speed * up;
		is_dirty = true;
	}

	if (matrices.frame <= max_samples)
		matrices.frame++;
	if (is_dirty) {
		matrices.frame = 0;
		update_vectors();
		is_dirty = false;
	}
	buffer->write_data(&matrices, sizeof(Matrices));

	if (show_window) {
		display_window();
	}
}
void Camera::update_vectors() {
	glm::vec3 direction;

	direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.z = sin(glm::radians(pitch));

	front = glm::normalize(direction);
	glm::vec3 right = glm::cross(front, glm::vec3(0.f, 0.f, -1.f));
	up = glm::cross(front, right);
	matrices.view = glm::lookAt(position, position + front, up);
	matrices.view_inverse = glm::inverse(matrices.view);
}

void Camera::display_window() {
	if (!ImGui::Begin("Camera", &show_window, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	}

	ImGui::InputFloat3("Position", &position[0]);
	ImGui::Separator();
	ImGui::InputFloat("yaw", &yaw, 0, 0, 3, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputFloat("pitch", &pitch, 0, 0, 3, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputInt("frame", &matrices.frame);
	ImGui::SliderInt("max samples", &max_samples, 0, 4096);
	ImGui::End();
}
