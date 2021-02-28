#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "nodes/Node.h"

#include <glm/gtx/quaternion.hpp>

class Node3D : public Node {
public:
	using Node::Node;

private:
	glm::mat4 local_transform = glm::mat4(1.0f);
	glm::mat4 world_transform = glm::mat4(1.0f);

public:
	glm::vec3 translation = glm::vec3(0.0);
	glm::quat rotation = glm::identity<glm::quat>();
	glm::vec3 scale = glm::vec3(1.0);

public:
	void translate(const glm::vec3 &translation);
	void rotate(const float angle, const glm::vec3 &dir);
	void move_to(const glm::vec3 &position);
	glm::mat4 const &get_local_transform() const { return local_transform; };
	glm::mat4 const &get_world_transform() const { return world_transform; };
	glm::vec3 const get_world_position() const { return glm::vec3(world_transform[3]); };
	glm::vec3 const get_local_position() const { return glm::vec3(local_transform[3]); };

	glm::vec3 forward() const { return glm::vec3(glm::transpose(local_transform)[2]); };
	glm::vec3 up() const { return glm::vec3(glm::transpose(local_transform)[1]); };
	glm::vec3 left() const { return glm::vec3(glm::transpose(local_transform)[0]); };
	glm::vec3 down() const { return up() * -1.0f; };
	glm::vec3 backward() const { return forward() * -1.0f; };
	glm::vec3 right() const { return left() * -1.0f; };

	void update_xform();

	// Inherited via Node
	virtual void load() override{};
	virtual void start() override{};
	virtual void update(float delta_time) override{};
	virtual void on_parent_changed() override { update_xform(); };
	virtual void draw_gui() override;
};
