#pragma once

#include "game/Terrain.h"
#include "nodes/Camera.h"
#include "nodes/Node3D.h"
#include "scene/Scene.h"

class Player : public Node3D {
	using Node3D::Node3D;

private:
	float yaw = 0.f;
	float pitch = 0.f;

	const float rotate_speed = 30.0f;
	const float move_speed = 1.f;

public:
	Camera *camera;
	Terrain *terrain;

	virtual void load(Scene &scene) override;
	virtual void start(Scene &scene) override;
	virtual void update(Scene &scene, float delta_time) override;
};
