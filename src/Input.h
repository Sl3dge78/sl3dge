#ifndef INPUT_H
#define INPUT_H
#include <memory>

#include <SDL/SDL.h>

class Input {
	inline static int numkeys;
	inline static Uint8 *keyboard_previous_frame = nullptr;
	inline static const Uint8 *keyboard;

	inline static int delta_mouse_x_, delta_mouse_y_;
	inline static uint32_t mouse_button_previous_frame;
	inline static uint32_t mouse_button;

public:
	static void start() {
		keyboard = SDL_GetKeyboardState(&numkeys);
		keyboard_previous_frame = new Uint8[numkeys];
	}
	static void update() {
		std::copy(keyboard, keyboard + numkeys, keyboard_previous_frame);

		mouse_button_previous_frame = mouse_button;
		mouse_button = SDL_GetRelativeMouseState(&delta_mouse_x_, &delta_mouse_y_);
	}
	static void cleanup() {
		delete[] keyboard_previous_frame;
	}

	static bool get_key(SDL_Scancode scancode) {
		return keyboard[scancode];
	}

	static bool get_key_down(SDL_Scancode scancode) {
		return keyboard[scancode] && !keyboard_previous_frame[scancode];
	}

	static bool get_key_up(SDL_Scancode scancode) {
		return !keyboard[scancode] && keyboard_previous_frame[scancode];
	}

	static bool get_mouse(int button) {
		return SDL_BUTTON(button) & mouse_button;
	}

	static bool get_mouse_down(int button) {
		return SDL_BUTTON(button) & mouse_button && !(SDL_BUTTON(button) & mouse_button_previous_frame);
	}

	static bool get_mouse_up(int button) {
		return !(SDL_BUTTON(button) & mouse_button) && (SDL_BUTTON(button) & mouse_button_previous_frame);
	}

	static int delta_mouse_x() {
		return delta_mouse_x_;
	}
	static int delta_mouse_y() {
		return delta_mouse_y_;
	}
};

#endif
