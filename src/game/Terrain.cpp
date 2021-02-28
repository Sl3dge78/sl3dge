#include "Terrain.h"

#include <imgui/imgui.h>

#include "Mathf.h"

float Terrain::get_height(const float x, const float y) {
	// Interpolate between a,b,c,d to get the real height

	this->x = x / scale;
	this->y = y / scale;

	float i_x;
	float i_y;
	float f_x = std::modf(Mathf::clamp(x / scale, 0.0f, width - 1.0001f), &i_x);
	float f_y = std::modf(Mathf::clamp(y / scale, 0.0f, height - 1.0001f), &i_y);

	h = get_world_position().z;

	// First we find the square we're in
	uint32_t a = i_x + width * i_y;
	uint32_t b = (i_x + 1) + width * i_y;
	uint32_t c = i_x + width * (i_y + 1);
	uint32_t d = (i_x + 1) + width * (i_y + 1);

	/* Smoother calculation based on edges 
	float hac = Mathf::lerp(mesh->heightmap[a], mesh->heightmap[c], f_y);
	float hbd = Mathf::lerp(mesh->heightmap[b], mesh->heightmap[d], f_y);

	return h = Mathf::lerp(hac, hbd, f_x);
	*/

	// Calculation based on face data
	bary = Mathf::barycentre(f_x, f_y);
	if (bary.z >= 0) { // On est dans ABC
		// a __ b
		//  |x/
		//  |/
		// c

		float ha = mesh->heightmap[a];
		float hb = mesh->heightmap[b];
		float hc = mesh->heightmap[c];

		h += (hb * bary.x) + (hc * bary.y) + (ha * bary.z);

	} else { // On est dans CBD
		// d __ c
		//  |x/
		//  |/
		// b
		bary = Mathf::barycentre(1.0f - f_x, 1.0f - f_y); // Recompute the barycentre relative to that triangle

		float hb = mesh->heightmap[b];
		float hc = mesh->heightmap[c];
		float hd = mesh->heightmap[d];

		h += (hc * bary.x) + (hb * bary.y) + (hd * bary.z);
	}
	return h;
}

void Terrain::update(float delta_time) {
	ImGui::Begin("Terrain");

	bool changed = false;

	changed |= ImGui::SliderFloat("x", &x, 0.0f, width - 1.0001f);
	changed |= ImGui::SliderFloat("y", &y, 0.0f, height - 1.0001f);
	if (changed)
		get_height(x, y);

	ImGui::LabelText("Height", "%.2f", h);
	ImGui::LabelText("Barycentre", "%.2f,%.2f,%.2f", bary.x, bary.y, bary.z);

	ImGui::End();
}
