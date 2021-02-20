#include "Terrain.h"
/*
float Terrain::get_height(const float x, const float y) {
	// Interpolate between a,b,c,d to get the real height
	// a ____ b
	//  | o  |
	//  |____|
	// c      d
	//
	// ABC barycenter
	float i_x;
	float i_y;
	float f_x = std::modf(x, &i_x);
	float f_y = std::modf(y, &i_y);

	float height = transform[3][3]; // Pas sur de ca ouech

	if (1.0f - f_x - f_y >= 0) { // On est dans ABC
		glm::vec3 bary = glm::vec3(f_x, f_y, 1.0f - f_x - f_y);

		float a = vertices[i_x + width * i_y].pos.z;
		float b = vertices[i_x + 1 + width * i_y].pos.z;
		float c = vertices[i_x + width * (i_y + 1)].pos.z;

		height += (a * bary.z) + (b * bary.x) + (c * bary.y);

	} else { // On est dans CBD
		// z = 1 - (1 - x) - (1 - y)
		// z = x + y - 1
		glm::vec3 bary = glm::vec3(1.0f - f_x, 1.0f - f_y, f_x + f_y - 1.0f); // Maths baybe

		float b = vertices[i_x + 1 + width * i_y].pos.z;
		float c = vertices[i_x + width * (i_y + 1)].pos.z;
		float d = vertices[i_x + 1 + width * (i_y + 1)].pos.z;

		height += (d * bary.z) + (b * bary.x) + (c * bary.y);
	}

	return height;
}
*/
