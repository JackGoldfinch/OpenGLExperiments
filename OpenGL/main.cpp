#pragma warning (disable: 4251)

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <glbinding/Binding.h>
#include <glbinding/gl46/gl.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

SDL_Window *window;
SDL_GLContext ctx;

bool running = true;

auto sdl_fullscreen_mode = SDL_WINDOW_FULLSCREEN;
Uint32 current_fullscreen_mode = 0;

#define evt_string(evt) {evt, #evt}

const std::unordered_map<Uint32, const char *> evt_strings {
	evt_string(SDL_QUIT),
	evt_string(SDL_KEYDOWN),
	evt_string(SDL_KEYUP)
};

void system_event(SDL_Event &evt) {
	SDL_MessageBoxButtonData mbbd[]{
				{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Quit"},
				{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel"}
	};

	SDL_MessageBoxData mbd;
	mbd.buttons = mbbd;
	mbd.numbuttons = 2;
	mbd.title = "QUIT";
	mbd.message = "Really quit?";
	mbd.window = window;
	mbd.flags = SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT | SDL_MESSAGEBOX_WARNING;
	mbd.colorScheme = nullptr;

	int button = 0;
	SDL_ShowMessageBox(&mbd, &button);

	if (button == 0) {
		running = false;
	}
}

const unsigned int max_key_combo_count = 4;

namespace std {
	string to_string(const unordered_set<SDL_Scancode> &set) {
		std::string str;

		if (set.size() == 0) return str;

		auto last = set.cend();
		last--;
		for (const auto &key : set) {
			str += std::to_string(key);

			if (key != *last) {
				str += ":";
			}
		}

		return str;
	}

	template<>
	struct hash<std::unordered_set<SDL_Scancode>> {
		size_t operator()(std::unordered_set<SDL_Scancode> const &set) const noexcept {
			return hash<std::string>{}(to_string(set));
		}
	};
}

std::unordered_map<std::unordered_set<SDL_Scancode>, void(*)()> key_combo_actions{
	{{SDL_SCANCODE_ESCAPE}, []() {
		SDL_Event evt;
		evt.type = SDL_QUIT;
		SDL_PushEvent(&evt);
	}},

	{{SDL_SCANCODE_LALT, SDL_SCANCODE_RETURN}, []() {
		if (current_fullscreen_mode == 0) {
			current_fullscreen_mode = sdl_fullscreen_mode;
			SDL_SetWindowFullscreen(window, sdl_fullscreen_mode);
		} else {
			current_fullscreen_mode = 0;
			SDL_SetWindowFullscreen(window, 0);
		}
	}}
};

struct keyboard_command_buffer {
	std::unordered_set<SDL_Scancode> keys;

	void operator()(const SDL_KeyboardEvent &evt) {
		switch (evt.type) {
			case SDL_KEYDOWN:
				if (!evt.repeat && keys.size() <= max_key_combo_count) {
					keys.insert(evt.keysym.scancode);
				}
				break;

			case SDL_KEYUP:
				keys.erase(evt.keysym.scancode);
				break;

			default:
				break;
		}

		try {
			auto func = key_combo_actions.at(keys);
			func();
		} catch (...) {

		}
	}
};

namespace std {
	std::string to_string(const keyboard_command_buffer &kcb) {
		return std::to_string(kcb.keys);
	}
}

keyboard_command_buffer kcb;

void keyboard_event(SDL_KeyboardEvent &evt) {
	kcb(evt);

	SDL_SetWindowTitle(window, std::to_string(kcb).c_str());
}

using event_func = void(*)(SDL_Event &);
const std::unordered_map<Uint32, event_func> events{
	{SDL_QUIT, system_event},
	{SDL_KEYUP,	[](SDL_Event &evt) {keyboard_event(evt.key); }},
	{SDL_KEYDOWN, [](SDL_Event &evt) {keyboard_event(evt.key); }}
};

int main(int argc, char *args[]) {
	if (SDL_Init(SDL_INIT_VIDEO)) {
		return -1;
	}

	auto num_displays = SDL_GetNumVideoDisplays();
	SDL_DisplayMode mdm;
	SDL_GetDesktopDisplayMode(0, &mdm);

	window = SDL_CreateWindow("OpenGL Experiments", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);
	if (!window) {
		std::cout << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_SetWindowDisplayMode(window, &mdm);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	ctx = SDL_GL_CreateContext(window);

	glbinding::Binding::initialize(nullptr);

	gl::glClearColor(1.f, 0.f, 0.f, 1.f);


	while (running) {

		SDL_Event evt;
		while (running && SDL_PollEvent(&evt)) {

			try {

				auto &func = events.at(evt.type);
				func(evt);

				std::cout << evt.common.timestamp << ": handled " << evt_strings.at(evt.type) << std::endl;

			} catch (...) {
				
			}

		}

		gl::glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(window);

	}



	SDL_GL_DeleteContext(ctx);

	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
