#pragma warning (disable: 4251)

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <glbinding/Binding.h>
#include <glbinding/gl46/gl.h>

#include <glm/glm.hpp>

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <fstream>
#include <filesystem>

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

template<gl::GLenum T>
struct shader_program {
	shader_program(const std::filesystem::path &path) {
		
		try {

			{
				auto size = std::filesystem::file_size(path);
				std::vector<char> buf(size + 1);
				std::ifstream file(path);
				file.read(buf.data(), size);

				const char *const pbuf = buf.data();
				id_ = gl::glCreateShaderProgramv(T, 1, &pbuf);
			}

			{
				gl::GLint is_linked = 0;
				gl::glGetProgramiv(id_, gl::GL_LINK_STATUS, &is_linked);

				if (!is_linked) {
					gl::GLint max_length = 0;
					gl::glGetProgramiv(id_, gl::GL_INFO_LOG_LENGTH, &max_length);

					if (max_length) {
						std::vector<char> info_log(max_length);
						gl::glGetProgramInfoLog(id_, max_length, &max_length, info_log.data());

						std::cout << "Program link error: " << info_log.data() << std::endl;
					}

					throw false;
				}
			}

			{
				gl::glValidateProgram(id_);

				gl::GLint is_valid = 0;
				gl::glGetProgramiv(id_, gl::GL_VALIDATE_STATUS, &is_valid);

				if (!is_valid) {
					gl::GLint max_length = 0;
					gl::glGetProgramiv(id_, gl::GL_INFO_LOG_LENGTH, &max_length);

					if (max_length) {
						std::vector<char> info_log(max_length);
						gl::glGetProgramInfoLog(id_, max_length, &max_length, info_log.data());

						std::cout << "Program validation error: " << info_log.data() << std::endl;
					}

					throw false;
				}

				{
					gl::GLint loc;
					gl::glGetUniformLocation(id_, "transform");
					
					gl::GLint uniforms_num;
					gl::glGetProgramiv(id_, gl::GL_ACTIVE_UNIFORMS, &uniforms_num);

					gl::GLint uniforms_len;
					gl::glGetProgramiv(id_, gl::GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniforms_len);

					std::vector<char> buf(uniforms_len);

					for (unsigned int i = 0; i < uniforms_num; ++i) {
						gl::glGetActiveUniformName(id_, i, uniforms_len, nullptr, buf.data());
					}
				}
			}
		} catch (...) {
			cleanup();
		}
	}

	~shader_program() {
		cleanup();
	}

	void cleanup() {
		gl::glDeleteProgram(id_);
		id_ = 0;
	}

	gl::GLint id_;
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

	gl::glViewport(0, 0, 640, 480);

	gl::glClearColor(1.f, 0.f, 0.f, 1.f);

	gl::GLuint vao;
	gl::glCreateVertexArrays(1, &vao);
	gl::glBindVertexArray(vao);

	glm::vec2 vertices[] = {
		{-.5f, -.5f},
		{.5f, -.5f},
		{0.f, .5f}
	};

	gl::GLuint vb;
	gl::glGenBuffers(1, &vb);
	gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vb);
	gl::glNamedBufferData(vb, 3 * sizeof(glm::vec2), vertices, gl::GL_STATIC_DRAW);

	gl::glEnableVertexAttribArray(0);
	gl::glVertexAttribPointer(0, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(glm::vec2), 0);

	std::filesystem::path vs_path{ "Shaders/default.vsh" };
	auto vs = std::make_unique<shader_program<gl::GL_VERTEX_SHADER>>(vs_path);

	std::filesystem::path fs_path{ "Shaders/default.fsh" };
	auto fs = std::make_unique<shader_program<gl::GL_FRAGMENT_SHADER>>(fs_path);

	gl::GLuint pp;
	gl::glCreateProgramPipelines(1, &pp);
	gl::glUseProgramStages(pp, gl::GL_VERTEX_SHADER_BIT, vs->id_);
	gl::glUseProgramStages(pp, gl::GL_FRAGMENT_SHADER_BIT, fs->id_);

	gl::glUseProgram(0);

	gl::glBindProgramPipeline(pp);

	glm::mat4 transform(1.f);
	gl::glProgramUniformMatrix4fv(vs->id_, 0, 1, gl::GL_FALSE, &transform[0][0]);
	

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

		gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);

		SDL_GL_SwapWindow(window);

	}

	gl::glDeleteProgramPipelines(1, &pp);

	gl::glDeleteBuffers(1, &vb);

	gl::glDeleteVertexArrays(1, &vao);

	fs.reset();

	//vs.reset();


	SDL_GL_DeleteContext(ctx);

	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
