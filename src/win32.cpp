// Global includes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <sl3dge/types.h>
#include <sl3dge/debug.h>
#include <sl3dge/module.h>

#include "game.h"
#include "vulkan.h"
#include <windows.h>

/*
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG

 IMPROVEMENTS

 IDEAS
 - In game console
*/

typedef struct ShaderCode {
	char *spv_path;
	FILETIME last_write_time;
} ShaderCode;

internal inline void *PlatformGetProcAddress(Module *module, const char *fn) {
	return GetProcAddress(module->dll, fn);
}

void VulkanLoadFunctions(Module *dll) {
	pfn_VulkanCreateContext =
			(fn_VulkanCreateContext *)PlatformGetProcAddress(dll, "VulkanCreateContext");
	pfn_VulkanDestroyContext =
			(fn_VulkanDestroyContext *)PlatformGetProcAddress(dll, "VulkanDestroyContext");
	pfn_VulkanReloadShaders =
			(fn_VulkanReloadShaders *)PlatformGetProcAddress(dll, "VulkanReloadShaders");
	pfn_VulkanDrawFrame = (fn_VulkanDrawFrame *)PlatformGetProcAddress(dll, "VulkanDrawFrame");
	pfn_VulkanDrawRTXFrame =
			(fn_VulkanDrawRTXFrame *)PlatformGetProcAddress(dll, "VulkanDrawRTXFrame");
	pfn_VulkanLoadScene = (fn_VulkanLoadScene *)PlatformGetProcAddress(dll, "VulkanLoadScene");
	pfn_VulkanFreeScene = (fn_VulkanFreeScene *)PlatformGetProcAddress(dll, "VulkanFreeScene");
}

void GameLoadFunctions(Module *dll) {
	pfn_GameStart = (fn_GameStart *)PlatformGetProcAddress(dll, "GameStart");
	pfn_GameLoop = (fn_GameLoop *)PlatformGetProcAddress(dll, "GameLoop");
}

internal void LogOutput(
		void *userdata, int category, SDL_LogPriority priority, const char *message) {
	FILE *std_err = fopen("bin/log.txt", "a");
	// fseek(std_err, 0, SEEK_END);
	fprintf(std_err, "%s\n", message);
	fclose(std_err);
}

internal int main(int argc, char *argv[]) {
#if DEBUG
	AllocConsole();
#endif

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *window = SDL_CreateWindow(
			"Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

	// Log to file
#ifndef DEBUG
	// Clear the log file
	FILE *std_err = fopen("bin/log.txt", "w+");
	fclose(std_err);
	SDL_LogSetOutputFunction(&LogOutput, NULL);

#endif

	Module vulkan_module = {};
	Win32LoadModule(&vulkan_module, "vulkan");
	VulkanLoadFunctions(&vulkan_module);

	VulkanContext *context = pfn_VulkanCreateContext(window);

	Module game_module = {};
	Win32LoadModule(&game_module, "game");
	GameLoadFunctions(&game_module);

	SDL_ShowWindow(window);

	ShaderCode shader_code = {};
	shader_code.spv_path = "resources\\shaders\\shaders.meta";
	shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);

	GameData game_data = {};
	Scene *scene;

	SDL_Log("Loading scene...");
	const char *scene_path = "resources/3d/Motorcycle/motorcycle.gltf";
	// const char *scene_path = "resources/models/gltf_samples/Sponza/glTF/Sponza.gltf";
	scene = pfn_VulkanLoadScene(scene_path, context);

	pfn_GameStart(&game_data);

	bool running = true;
	float delta_time = 0;
	int frame_start = SDL_GetTicks();

	SDL_Event event;
	while (running) {
		{
			// Sync
			int time = SDL_GetTicks();
			delta_time = (float)(time - frame_start) / 1000.f;
			frame_start = time;
		}

		// Reload vulkan if necessary
		if (Win32ShouldReloadModule(&vulkan_module)) {
			pfn_VulkanFreeScene(context, scene);
			pfn_VulkanDestroyContext(context);
			Win32CloseModule(&vulkan_module);

			Win32LoadModule(&vulkan_module, "vulkan");
			VulkanLoadFunctions(&vulkan_module);
			context = pfn_VulkanCreateContext(window);
			scene = pfn_VulkanLoadScene(scene_path, context);
			SDL_Log("Vulkan reloaded");
		}

		// Reload gamecode if necessary
		if (Win32ShouldReloadModule(&game_module)) {
			Win32CloseModule(&game_module);
			Win32LoadModule(&game_module, "game");
			GameLoadFunctions(&game_module);
			SDL_Log("Game code reloaded");
		}

		// Reload shaders if necessary
		FILETIME shader_time = Win32GetLastWriteTime(shader_code.spv_path);
		if (CompareFileTime(&shader_code.last_write_time, &shader_time)) {
			shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);
			pfn_VulkanReloadShaders(context, scene);
			SDL_Log("Shaders reloaded");
		}

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				SDL_Log("Quit!");
				SDL_HideWindow(window);
				running = false;
			}
		}
		pfn_GameLoop(delta_time, &game_data);
		pfn_VulkanDrawFrame(context, scene, &game_data);

		{
			// 60 fps cap

			i32 now = SDL_GetTicks();
			i32 frame_time_ms = now - frame_start;
			char title[40];
			f32 idle_percent = frame_time_ms / (1000.0f / 60.0f);
			idle_percent = 1 - idle_percent;
			idle_percent *= 100.0f;
			snprintf(title, 40, "Frametime : %dms Idle: %2.f%%", frame_time_ms, idle_percent);
			SDL_SetWindowTitle(window, title);

			while (SDL_GetTicks() - frame_start < 1000.0f / 60.f) {
				// SDL_Log("%.4f", (SDL_GetTicks() - frame_start) / 1000.f);
			}
		}
	}

	pfn_VulkanFreeScene(context, scene);
	pfn_VulkanDestroyContext(context);

	Win32CloseModule(&game_module);

	Win32CloseModule(&vulkan_module);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	DBG_END();
	return (0);
}