#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "nodes/Node.h"

class Node3D : public Node {
public:
	using Node::Node;

private:
	glm::mat4 transform = glm::mat4(1.0f);

public:
	void translate(glm::vec3 translation) {
		transform = glm::translate(transform, translation);
	}
	void rotate(const float angle, const glm::vec3 dir) {
		transform = glm::rotate(transform, angle, dir);
	}
	glm::mat4 const &get_transform() const { return transform; };
};
