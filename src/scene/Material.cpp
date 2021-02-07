#include "Material.h"

#include "vulkan/VulkanApplication.h"

Texture::Texture(VulkanApplication &app, const std::string &texture_path) {
	SDL_Surface *surf = IMG_Load(texture_path.c_str());
	surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR8888, 0);

	if (surf == nullptr) {
		SDL_LogError(0, "%s", SDL_GetError());
		throw std::runtime_error("Unable to create texture");
	}

	vk::Format format(get_vk_format(surf->format));
	texture = std::unique_ptr<Image>(new Image(app, surf->w, surf->h, format, vk::ImageTiling::eOptimal, { vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled }, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor));

	// Create data transfer buffer
	vk::DeviceSize img_size = surf->w * surf->h * surf->format->BytesPerPixel;
	Buffer staging_buffer(app, img_size, { vk::BufferUsageFlagBits::eTransferSrc }, { vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent });
	staging_buffer.write_data(surf->pixels, img_size);

	// Copy the buffer to the image and send it to graphics queue
	auto transfer_cbuffer = app.create_commandbuffer(vk::QueueFlagBits::eTransfer, true);
	texture->transition_layout(transfer_cbuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, app.get_transfer_family_index(), app.get_graphics_family_index());
	copy_buffer_to_image(transfer_cbuffer, staging_buffer.buffer, texture->image, surf->w, surf->h);
	texture->transition_layout(transfer_cbuffer, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, app.get_transfer_family_index(), app.get_graphics_family_index());

	// Receive the image on the graphics queue
	auto graphics_cbuffer = app.create_commandbuffer();
	vk::ImageMemoryBarrier barrier(
			{}, vk::AccessFlagBits::eVertexAttributeRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
			app.get_transfer_family_index(), app.get_graphics_family_index(),
			texture->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
	graphics_cbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eVertexInput, {}, nullptr, nullptr, barrier);

	// Wait for everything to process
	app.flush_commandbuffer(transfer_cbuffer, vk::QueueFlagBits::eTransfer, true);
	app.flush_commandbuffer(graphics_cbuffer);

	SDL_FreeSurface(surf);
}

bool Material::on_gui() {
	if (!draw_ui)
		return false;

	bool changed = false;
	ImGui::Begin("Material", &draw_ui);
	changed |= ImGui::ColorPicker3("Albedo", &albedo.r);
	changed |= ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Metallic", &metallic, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("AO", &ao, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Rim Pow", &rim_pow, 1.00f, 0.0f, 64.0f);
	changed |= ImGui::DragFloat("Rim Strength", &rim_strength, 0.01f, 0.0f, 10.0f);
	ImGui::End();
	return changed;
}
