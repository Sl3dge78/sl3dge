#include "Node3D.h"

#include <SDL/SDL.h>

void Node3D::translate(const glm::vec3 &t) {
	this->translation += t;

	update_xform();
	notify_children();
}

void Node3D::rotate(const float angle, const glm::vec3 &dir) {
	this->rotation = glm::rotate(this->rotation, glm::radians(angle), dir);
	update_xform();
	notify_children();
}

void Node3D::update_xform() {
	glm::mat4 t = glm::translate(glm::mat4(1.0), translation);
	glm::mat4 r = glm::toMat4(rotation);
	glm::mat4 s = glm::scale(glm::mat4(1.0), scale);

	//local_transform = s * r * t;
	local_transform = t * r * s;

	Node3D *par = dynamic_cast<Node3D *>(parent);
	if (par != nullptr) {
		world_transform = par->get_world_transform() * local_transform;
	}
}
