#pragma once

#include <string>
#include <vector>

class Scene;

class Node {
	friend class Scene;

public:
	std::vector<Node *> children;
	Node *parent;
	std::string name = "Node";

	Node(Node *parent) {
		this->parent = parent;
		children = std::vector<Node *>();
		if (parent != nullptr)
			parent->children.push_back(this);
	};
	virtual ~Node() = default;

	virtual void load() = 0;
	virtual void start() = 0;
	virtual void update(float delta_time) = 0;
	virtual void on_parent_changed() = 0;
	virtual void draw_gui() = 0;

protected:
	Scene *scene = nullptr;

	void notify_children() {
		for (auto &c : children) {
			c->on_parent_changed();
		}
	}
};
