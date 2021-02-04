#ifndef CAMERA_H
#define CAMERA_H

#include <SDL/SDL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Input.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;

class Camera {
private:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;

	const float camera_speed = 100.0f;
	const float move_speed = 1.0f;
	float yaw = 0.f;
	float pitch = 0.f;

	float aspect_ratio = 1280.0f / 720.0f;
	uint32_t height;
	bool is_dirty;
	int max_samples = 16;

public:
	struct Matrices {
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 view_inverse;
		alignas(16) glm::mat4 proj_inverse;
		alignas(4) int frame;
	} matrices;

	std::unique_ptr<Buffer> buffer;

	void load(VulkanApplication *app);
	void start();
	void update(float delta_time);

	void update_vectors();
	glm::vec3 get_position() const { return position; }

	void display_window();

	bool show_window = false;
};

#endif
