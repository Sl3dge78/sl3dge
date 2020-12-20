#include "Camera.h"

#include "imgui/imgui.h"
#include "imgui_custom.h"

void Camera::start() {
	position = glm::vec3(1.5f, 1.5f, 1.f);

	yaw = 220.f;
	pitch = -8.f;

	update_vectors();
}

void Camera::update(float delta_time) {
	if (Input::get_mouse(3)) {
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

		update_vectors();
	}

	float delta_speed = delta_time * move_speed;

	if (Input::get_key(SDL_SCANCODE_LSHIFT))
		delta_speed *= 2.0f;

	if (Input::get_key(SDL_SCANCODE_W))
		position += delta_speed * front;
	if (Input::get_key(SDL_SCANCODE_S))
		position -= delta_speed * front;
	if (Input::get_key(SDL_SCANCODE_A))
		position -= delta_speed * glm::normalize(glm::cross(front, up));
	if (Input::get_key(SDL_SCANCODE_D))
		position += delta_speed * glm::normalize(glm::cross(front, up));
	if (Input::get_key(SDL_SCANCODE_Q))
		position -= delta_speed * up;
	if (Input::get_key(SDL_SCANCODE_E))
		position += delta_speed * up;

	if (show_window) {
		if (!ImGui::Begin("Camera", &show_window, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::End();
			return;
		}

		ImGui::InputFloat3("Position", &position[0]);
		ImGui::Separator();
		ImGui::InputFloat("yaw", &yaw, 0, 0, 3, ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat("pitch", &pitch, 0, 0, 3, ImGuiInputTextFlags_ReadOnly);
		ImGui::End();
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
}

void Camera::get_view_matrix(glm::mat4 &view) {
	view = glm::lookAt(position, position + front, up);
	//view = glm::lookAt(position, front, up);
}
