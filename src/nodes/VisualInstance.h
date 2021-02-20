#pragma once

#include <vulkan/vulkan.hpp>

#include "Node3D.h"

class VisualInstance : public Node3D {
public:
	VisualInstance(Node *parent) :
			Node3D(parent) {
		instance_id = VisualInstance::get_next_instance_id();
	};

	virtual void draw(vk::CommandBuffer cmd) = 0;

protected:
	static uint32_t get_next_instance_id() { return next_instance_id++; };
	uint32_t get_instance_id() { return instance_id; };

private:
	static uint32_t next_instance_id;
	uint32_t instance_id;
};
