#pragma once

#include "game/Terrain.h"
#include "nodes/Camera.h"
#include "nodes/Node3D.h"
#include "scene/Scene.h"

class Player : public Node3D {
	using Node3D::Node3D;

private:
	float yaw = 0.f;
	float pitch = 0.f;

	glm::vec3 pos = glm::vec3(1.0f);
	const float rotate_speed = 1.0f;
	const float move_speed = 1.0f;

	bool is_dirty = false;

public:
	Camera *camera;
	Terrain *terrain;

	virtual void load(Scene &scene) override {
	}

	virtual void start(Scene &scene) override {
		yaw = 220.f;
		pitch = -8.f;

		glm::vec3 direction;
		direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.z = sin(glm::radians(pitch));
		look_at(pos, direction + pos, glm::vec3(0.0, 0.0, 1.0));
	}

	virtual void update(Scene &scene, float delta_time) override {
		if (Input::get_mouse(3)) {
			if (Input::delta_mouse_x() != 0 || Input::delta_mouse_y() != 0) {
				is_dirty = true;
			}
			yaw += Input::delta_mouse_x() * rotate_speed * 100.0f * delta_time;
			pitch -= Input::delta_mouse_y() * rotate_speed * 100.0f * delta_time;

			if (yaw > 360)
				yaw = 0;
			if (yaw < 0)
				yaw = 360;

			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;

			float delta_speed = delta_time * move_speed;
			glm::vec3 translation(0.0f);

			if (Input::get_key(SDL_SCANCODE_LSHIFT)) {
				delta_speed *= 2.0f;
			}

			if (Input::get_key(SDL_SCANCODE_W)) {
				translation -= forward();
			}
			if (Input::get_key(SDL_SCANCODE_S)) {
				translation += forward();
			}
			if (Input::get_key(SDL_SCANCODE_A)) {
				translation -= right();
			}
			if (Input::get_key(SDL_SCANCODE_D)) {
				translation += right();
			}
			if (Input::get_key(SDL_SCANCODE_Q)) {
				translation -= up();
			}
			if (Input::get_key(SDL_SCANCODE_E)) {
				translation += up();
			}

			if (translation.length() > 0) {
				pos += translation * delta_speed;
				is_dirty = true;
			}

			glm::vec3 direction;
			direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.z = sin(glm::radians(pitch));
			look_at(pos, direction + pos, glm::vec3(0.0, 0.0, 1.0));
			is_dirty = false;
		}
	}
};
