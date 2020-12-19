#ifndef CAMERA_OBJ_H
#define CAMERA_OBJ_H

#include <SDL/SDL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;

	const float camera_speed = 10.0f;
	const float move_speed = 1.0f;
	float yaw = 0.f;
	float pitch = 0.f;

public:
	void start();
	void update(float delta_time);
	void get_view_matrix(glm::mat4 &view);
};

#endif
