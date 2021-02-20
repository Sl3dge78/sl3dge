#pragma once

#include <vector>

class Node {
public:
	std::vector<Node *> children;
	Node *parent;

	Node(Node *parent) {
		this->parent = parent;
		children = std::vector<Node *>();
		if (parent != nullptr)
			parent->children.push_back(this);
	};
	virtual ~Node() = default;
};
