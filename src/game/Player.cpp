#include "Player.h"

void Player::load(Scene &scene) {
}

void Player::start(Scene &scene) {
}

void Player::update(Scene &scene, float delta_time) {
	if (Input::get_mouse(3)) {
		camera->rotate(-Input::delta_mouse_x() * rotate_speed * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
		camera->rotate(Input::delta_mouse_y() * rotate_speed * delta_time, camera->left());
	}

	glm::vec3 translation(0.0f);
	if (Input::get_key(SDL_SCANCODE_W)) {
		translation += camera->forward();
	}
	if (Input::get_key(SDL_SCANCODE_S)) {
		translation -= camera->forward();
	}
	if (Input::get_key(SDL_SCANCODE_A)) {
		translation += camera->left();
	}
	if (Input::get_key(SDL_SCANCODE_D)) {
		translation -= camera->left();
	}
	if (Input::get_key(SDL_SCANCODE_Q)) {
		translation += camera->up();
	}
	if (Input::get_key(SDL_SCANCODE_E)) {
		translation -= camera->up();
	}
	translation *= move_speed;
	auto gravity = glm::vec3(0.0, 0.0, -1.0) * 9.81f;
	translation += gravity;

	translation *= delta_time;

	auto new_pos = get_world_position() + translation;

	if (new_pos.z <= 0)
		new_pos.z = 0;

	this->move_to(new_pos);
}
