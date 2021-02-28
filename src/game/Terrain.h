#pragma once
#include "nodes/MeshInstance.h"

/// Terrains are meshes where the vertices are arranged in a grid. Only the height really matters
class Terrain : public MeshInstance {
private:
	uint32_t width, height;
	float x = 0.0f, y = 0.0f;
	float h = 0.0f;
	glm::vec3 bary = glm::vec3(0.0);
	float scale = 5.0f;

public:
	Terrain(Node *parent, Mesh *mesh, Material *material, const uint32_t width, const uint32_t height) :
			MeshInstance(parent, mesh, material), width(width), height(height){};
	~Terrain() = default;

	float get_height(const float x, const float y);

	virtual void update(float delta_time) override;
};
