#pragma once

#include "nodes/VisualInstance.h"
#include "scene/Material.h"
#include "vulkan/Mesh.h"

class MeshInstance : public VisualInstance {
protected:
	Mesh *mesh;
	Material *material;

public:
	MeshInstance(Node *node, Mesh *mesh, Material *material) :
			VisualInstance(node), mesh(mesh), material(material){};

	vk::AccelerationStructureInstanceKHR get_ASInstance_data() {
		vk::AccelerationStructureInstanceKHR as_instance = {};
		as_instance.flags = (unsigned int)(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);
		as_instance.instanceShaderBindingTableRecordOffset = 0;
		as_instance.instanceCustomIndex = get_instance_id();
		as_instance.accelerationStructureReference = mesh->blas->get_address();
		as_instance.mask = 0xFF;
		auto transposed = glm::transpose(this->get_world_transform());
		memcpy(&as_instance, &transposed, sizeof(vk::TransformMatrixKHR));
		return as_instance;
	}

	// Inherited via VisualInstance
	virtual void draw(vk::CommandBuffer cmd) override;
	uint32_t get_mesh_id() const {
		return mesh->get_id();
	};
	uint32_t get_material_id() const {
		return material->get_id();
	};
};
