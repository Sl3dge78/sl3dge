#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "glm/glm.hpp"

using namespace ImGui;

bool InputVec3(const char *label, glm::vec3 *vec, const float step = NULL, const float step_fast = NULL, const char *format = NULL, ImGuiInputTextFlags flags = 0) {
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext &g = *GImGui;
	bool value_changed = false;
	BeginGroup();
	PushID(label);
	PushMultiItemsWidths(3, CalcItemWidth());

	PushID(0);

	value_changed |= InputFloat("", &vec->x, step, step_fast, format, flags);
	PopID();
	PopItemWidth();

	PushID(1);

	SameLine(0, g.Style.ItemInnerSpacing.x);
	value_changed |= InputFloat("", &vec->y, step, step_fast, format, flags);
	PopID();
	PopItemWidth();

	PushID(2);

	SameLine(0, g.Style.ItemInnerSpacing.x);
	value_changed |= InputFloat("", &vec->z, step, step_fast, format, flags);
	PopID();
	PopItemWidth();

	PopID();

	const char *label_end = FindRenderedTextEnd(label);
	if (label != label_end) {
		SameLine(0.0f, g.Style.ItemInnerSpacing.x);
		TextEx(label, label_end);
	}

	EndGroup();
	return value_changed;
}
