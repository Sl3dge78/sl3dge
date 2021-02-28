#include "Player.h"

void Player::load(Scene &scene) {
}

void Player::start(Scene &scene) {
}

void Player::update(Scene &scene, float delta_time) {
	if (Input::get_mouse(3)) {
		rotate(-Input::delta_mouse_x() * rotate_speed * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
		rotate(Input::delta_mouse_y() * rotate_speed * delta_time, left());

		yaw += Input::delta_mouse_x() * rotate_speed * delta_time;
		pitch -= Input::delta_mouse_y() * rotate_speed * delta_time;

		if (yaw > 360)
			yaw = 0;
		if (yaw < 0)
			yaw = 360;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		glm::vec3 direction;
		direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.z = sin(glm::radians(pitch));
		//look_at(direction);
	}
	float delta_speed = delta_time * move_speed;
	glm::vec3 translation(0.0f);

	if (Input::get_key(SDL_SCANCODE_LSHIFT)) {
		delta_speed *= 2.0f;
	}

	if (Input::get_key(SDL_SCANCODE_W)) {
		translation += forward();
	}
	if (Input::get_key(SDL_SCANCODE_S)) {
		translation -= forward();
	}
	if (Input::get_key(SDL_SCANCODE_A)) {
		translation += left();
	}
	if (Input::get_key(SDL_SCANCODE_D)) {
		translation -= left();
	}
	if (Input::get_key(SDL_SCANCODE_Q)) {
		translation += up();
	}
	if (Input::get_key(SDL_SCANCODE_E)) {
		translation -= up();
	}

	/*
	if (Input::get_key(SDL_SCANCODE_W)) {
		translation += glm::vec3(0.0, 1.0, 0.0);
	}
	if (Input::get_key(SDL_SCANCODE_S)) {
		translation += glm::vec3(0.0, -1.0, 0.0);
	}
	if (Input::get_key(SDL_SCANCODE_A)) {
		translation += glm::vec3(-1.0, 0.0, 0.0);
	}
	if (Input::get_key(SDL_SCANCODE_D)) {
		translation += glm::vec3(1.0, 0.0, 0.0);
	}
	if (Input::get_key(SDL_SCANCODE_Q)) {
		translation += glm::vec3(0.0, 0.0, -1.0);
	}
	if (Input::get_key(SDL_SCANCODE_E)) {
		translation += glm::vec3(0.0, 0.0, 1.0);
	}
	*/
	if (translation.length() > 0) {
		translate(translation * delta_speed);
	}
}
