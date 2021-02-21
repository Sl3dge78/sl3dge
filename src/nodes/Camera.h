#ifndef CAMERA_H
#define CAMERA_H

#include <SDL/SDL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Input.h"
#include "nodes/Node3D.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;

// Todo switch that as a node ?
class Camera : public Node3D {
public:
	Camera(Node *parent) :
			Node3D(parent){};

private:
	const float aspect_ratio = 1280.0f / 720.0f;
	uint32_t height;
	bool is_dirty;
	int max_samples = 8;

public:
	struct Matrices {
		alignas(16) glm::mat4 view = glm::mat4(1.0);
		alignas(16) glm::mat4 proj = glm::mat4(1.0);
		alignas(16) glm::mat4 view_inverse = glm::mat4(1.0);
		alignas(16) glm::mat4 proj_inverse = glm::mat4(1.0);
		alignas(16) glm::vec3 pos = glm::vec3(0.0f);
		alignas(4) int frame = 0;
	} matrices;

	void start(Scene &scene) override;
	void update(Scene &scene, float delta_time) override;
	virtual void on_parent_changed() override;

	void display_window();

	bool show_window = false;
};

#endif
