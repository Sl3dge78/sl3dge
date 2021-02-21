#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "nodes/Node.h"

class Node3D : public Node {
public:
	using Node::Node;

private:
	glm::mat4 local_transform = glm::mat4(1.0f);
	glm::mat4 world_transform = glm::mat4(1.0f);

protected:
public:
	void translate(glm::vec3 translation) {
		local_transform = glm::translate(local_transform, translation);
		update_world_xform();
		notify_children();
	}
	void rotate(const float angle, const glm::vec3 dir) {
		local_transform = glm::rotate(local_transform, angle, dir);
		update_world_xform();
		notify_children();
	}
	void look_at(const glm::vec3 &pos, const glm::vec3 &forward, const glm::vec3 &up) {
		local_transform = glm::lookAt(pos, forward, up);
		update_world_xform();
		notify_children();
	}
	glm::mat4 const &get_local_transform() const { return local_transform; };
	glm::mat4 const &get_world_transform() const { return world_transform; };
	glm::vec3 const get_position() const { return glm::vec3(world_transform[3]); };
	glm::vec3 const get_local_position() const { return glm::vec3(local_transform[3]); };

	glm::vec3 forward() const { return glm::vec3(glm::transpose(local_transform)[2]); };
	glm::vec3 up() const { return glm::vec3(glm::transpose(local_transform)[1]); };
	glm::vec3 right() const { return glm::vec3(glm::transpose(local_transform)[0]); };

	void update_world_xform() {
		Node3D *par = dynamic_cast<Node3D *>(parent);
		if (par != nullptr) {
			world_transform = par->get_world_transform() * local_transform;
		}
	};
	// Inherited via Node
	virtual void load(Scene &scene) override{};
	virtual void start(Scene &scene) override{};
	virtual void update(Scene &scene, float delta_time) override{};
	virtual void on_parent_changed() override {
		update_world_xform();
	};
};
