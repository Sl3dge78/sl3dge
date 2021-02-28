#pragma once

#include <glm/glm.hpp>

class Mathf {
public:
	static float clamp(const float v, const float min, const float max) {
		if (v > max)
			return max;
		else if (v < min)
			return min;
		else
			return v;
	};
	static glm::vec3 barycentre(const float x, const float y) {
		return glm::vec3(x, y, 1.0f - x - y);
	};
	static float lerp(const float min, const float max, const float t) {
		return min + ((max - min) * t);
	}
};
