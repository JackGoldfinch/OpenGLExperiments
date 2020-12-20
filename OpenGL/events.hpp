#pragma once

#define evt_string(evt) {evt, #evt}

const std::unordered_map<Uint32, const char *> evt_strings{
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

struct SCANCODE_STRING_ {
	const char *operator[](Uint32 scancode) const noexcept {
		try {
			return strings.at(scancode);
		} catch (...) {
			return nullptr;
		}
	}

	static const std::unordered_map<Uint32, const char *> strings;
} SCANCODE_STRING;

#define ENTRY(code,text) {SDL_SCANCODE_##code, text}

const std::unordered_map<Uint32, const char *> SCANCODE_STRING_::strings{
	ENTRY(ESCAPE, "ESC"),
	ENTRY(LALT, "L_ALT"),
	ENTRY(RETURN, "RETURN"),
	ENTRY(SPACE, "SPACE"),
	ENTRY(A, "A"),
	ENTRY(B, "B")
};

#undef ENTRY

const unsigned int max_key_combo_count = 4;

namespace std {
	string to_string(const unordered_set<SDL_Scancode> &set) {
		std::string str;

		if (set.size() == 0) return str;

		auto last = set.cend();
		last--;
		for (const auto &key : set) {
			auto scstr = SCANCODE_STRING[key];

			str += (scstr ? scstr : std::to_string(key));

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
		if (settings.current_fullscreen_mode == 0) {
			settings.current_fullscreen_mode = settings.fullscreen_mode;
			setup_fullscreen_mode();
			SDL_SetWindowFullscreen(window, settings.fullscreen_mode);
			SDL_SetWindowDisplayMode(window, &settings.roaming.mode);
		} else {
			settings.current_fullscreen_mode = 0;
			setup_windowed_mode();
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
