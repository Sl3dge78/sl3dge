#include "Debug.h"

void debug_begin_label(vk::CommandBuffer cmd_buf, const std::string label, std::array<float, 4> color) {
#ifdef _DEBUG
	cmd_buf.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT(label.c_str(), color));
#endif // !_DEBUG
}

void debug_end_label(vk::CommandBuffer cmd_buf) {
#ifdef _DEBUG
	cmd_buf.endDebugUtilsLabelEXT();
#endif // !_DEBUG
}

void debug_name_object(vk::Device device, const uint64_t handle, vk::ObjectType type, const char *name) {
	vk::DebugUtilsObjectNameInfoEXT name_info(type, uint64_t(handle), name);
	device.setDebugUtilsObjectNameEXT(name_info);
}
