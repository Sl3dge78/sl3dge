#pragma once

#include <vector>

class Scene;

class Node {
public:
	std::vector<Node *> children;
	Node *parent;
	bool dirty = false;
	Node(Node *parent) {
		this->parent = parent;
		children = std::vector<Node *>();
		if (parent != nullptr)
			parent->children.push_back(this);
	};
	virtual ~Node() = default;

	virtual void load(Scene &scene) = 0;
	virtual void start(Scene &scene) = 0;
	virtual void update(Scene &scene, float delta_time) = 0;
	virtual void on_parent_changed() = 0;

protected:
	void notify_children() {
		for (auto &c : children) {
			c->on_parent_changed();
		}
	}
};
