#pragma warning (disable: 4251)

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <glbinding/Binding.h>
#include <glbinding/gl46/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
namespace pt = boost::property_tree;

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <fstream>
#include <filesystem>
#include <initializer_list>
#include <chrono>

#include "glbuffers.hpp"

struct matrices_data : data {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;

	matrices_data():
	projection(1.f),
	view(1.f),
	model(1.f) {}
};

struct matrices : uniform_buffer_object<matrices_data> {
	matrices(gl::GLuint binding):uniform_buffer_object(binding) {}

	void update_projection(const glm::mat4 &projection) {
		update(offsetof(matrices_data, projection), sizeof(glm::mat4), &projection[0][0]);
	}

	void update_model(const glm::mat4 &model) {
		update(offsetof(matrices_data, model), sizeof(glm::mat4), &model[0][0]);
	}

	void update_view(const glm::mat4 &view) {
		update(offsetof(matrices_data, view), sizeof(glm::mat4), &view[0][0]);
	}
};

struct settings_ {
	SDL_WindowFlags fullscreen_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
	Uint32 current_fullscreen_mode = 0;

	struct general_video_ {
		bool fullscreen_window;

		struct video_ {
			gl::GLuint width, height;
		};

		struct windowed_ : video_ {

		} windowed;

		struct fullscreen_ : video_ {

		} fullscreen;
	} video;

	struct roaming_ {
		SDL_DisplayMode mode;
		std::unique_ptr<matrices> matrices;
	} roaming;

	void read() {
		pt::ptree file;
		pt::read_info("settings.conf", file);

		auto settings = file.get_child("settings");

		auto video = settings.get_child("video");

		this->video.fullscreen_window = video.get<bool>("fullscreen_window", false);

		auto windowed = video.get_child("windowed");
		this->video.windowed.width = windowed.get<gl::GLuint>("width", 1280);
		this->video.windowed.height = windowed.get<gl::GLuint>("height", 1024);

		auto fullscreen = video.get_child("fullscreen");
		this->video.fullscreen.width = fullscreen.get<gl::GLuint>("width", 1280);
		this->video.fullscreen.height = fullscreen.get<gl::GLuint>("height", 1024);
	}

	void write() {
		pt::ptree windowed;
		windowed.add("width", this->video.windowed.width);
		windowed.add("height", this->video.windowed.height);

		pt::ptree fullscreen;
		fullscreen.add("width", this->video.fullscreen.width);
		fullscreen.add("height", this->video.fullscreen.height);

		pt::ptree video;
		video.add("fullscreen_window", this->video.fullscreen_window);
		video.add_child("windowed", windowed);
		video.add_child("fullscreen", fullscreen);

		pt::ptree settings;
		settings.add_child("video", video);

		pt::ptree file;
		file.add_child("settings", settings);

		pt::write_info("settings.conf", file);
	}

} settings;

void setup_windowed_mode() {
	gl::glViewport(0, 0, settings.video.fullscreen.width, settings.video.fullscreen.height);

	auto w2 = settings.video.windowed.width / 2.f;
	auto h2 = settings.video.windowed.height / 2.f;

	//auto projection = glm::ortho(-w2, w2, -h2, h2, -1000.f, 1000.f);
	//settings.roaming.matrices->update_projection(projection);
}

void setup_fullscreen_mode() {
	gl::glViewport(0, 0, settings.video.fullscreen.width, settings.video.fullscreen.height);

	auto w2 = settings.video.fullscreen.width / 2.f;
	auto h2 = settings.video.fullscreen.height / 2.f;

	auto projection = glm::ortho(-w2, w2, -h2, h2, -1000.f, 1000.f);
	settings.roaming.matrices->update_projection(projection);
}

SDL_Window *window;
SDL_GLContext ctx;

bool running = true;

#include "events.hpp"

#include "shaders.hpp"

int main(int argc, char *args[]) {
	if (SDL_Init(SDL_INIT_VIDEO)) {
		return -1;
	}

	auto num_displays = SDL_GetNumVideoDisplays();
	SDL_GetDesktopDisplayMode(0, &settings.roaming.mode);

	settings.roaming.mode.refresh_rate = 150;

	auto mode = SDL_GetClosestDisplayMode(0, &settings.roaming.mode, &settings.roaming.mode);

	settings.read();

	settings.video.fullscreen.width = settings.roaming.mode.w;
	settings.video.fullscreen.height = settings.roaming.mode.h;

	Uint32 window_flags = SDL_WINDOW_OPENGL;

	if (settings.video.fullscreen_window) {
		window_flags = SDL_WINDOW_OPENGL | settings.fullscreen_mode;
	}

	window = SDL_CreateWindow("OpenGL Experiments", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, settings.video.windowed.width, settings.video.windowed.height, window_flags);
	if (!window) {
		std::cout << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_SetWindowDisplayMode(window, &settings.roaming.mode);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	ctx = SDL_GL_CreateContext(window);

	glbinding::Binding::initialize(nullptr);

	gl::glEnable(gl::GL_DEPTH_TEST);
	gl::glDepthFunc(gl::GL_ALWAYS);

	gl::glDisable(gl::GL_CULL_FACE);

	settings.roaming.matrices = std::make_unique<matrices>(0);
	settings.roaming.matrices->update();

	setup_windowed_mode();

	gl::glClearColor(1.f, 0.f, 0.f, 1.f);
	gl::glClearDepth(1000.f);

	gl::GLuint vao;
	gl::glCreateVertexArrays(1, &vao);
	gl::glBindVertexArray(vao);

	glm::vec2 vertices[] = {
		{-3.f, -1.f},
		{3.f, -1.f},
		{0.f, 2.f}
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

	auto pp = std::make_unique<shader_pipeline>(shader_pipeline::init_list{vs.get(), fs.get()});
	pp->bind();

	gl::GLuint fb;
	gl::glCreateFramebuffers(1, &fb);

	gl::GLuint cb;
	gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &cb);
	gl::glBindTexture(gl::GL_TEXTURE_2D, cb);
	gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA16F, settings.video.fullscreen.width, settings.video.fullscreen.height, 0, gl::GL_RGBA, gl::GL_FLOAT, nullptr);
	gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
	gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
	gl::glNamedFramebufferTexture(fb, gl::GL_COLOR_ATTACHMENT0, cb, 0);

	gl::GLuint db;
	gl::glCreateRenderbuffers(1, &db);
	gl::glNamedRenderbufferStorage(db, gl::GL_DEPTH_COMPONENT16, settings.video.fullscreen.width, settings.video.fullscreen.height);
	gl::glNamedFramebufferRenderbuffer(fb, gl::GL_DEPTH_ATTACHMENT, gl::GL_RENDERBUFFER, db);

	if (gl::glCheckFramebufferStatus(gl::GL_FRAMEBUFFER) != gl::GL_FRAMEBUFFER_COMPLETE) {
		auto error = gl::glGetError();

		std::cout << "Framebuffer not complete: " << static_cast<gl::GLint>(error) << std::endl;
	}

	gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, fb);

	//settings.roaming.matrices->update_projection(glm::perspective(glm::radians(86.f), 16.f / 9.f, 0.f, 100.f));

	float delta_time = 0.f;
	auto last_frame = std::chrono::high_resolution_clock::now();

	glm::mat4 rotate(1.f);

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

		gl::glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);

		/*auto rotation = delta_time * 90.f;
		rotate = glm::rotate(rotate, glm::radians(rotation), glm::vec3{0.f, 1.f, 0.f});
		settings.roaming.matrices->update_model(rotate);*/

		gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);

		gl::glBlitNamedFramebuffer(fb, 0, 0, 0, 2560, 1440, 0, 0, 1280, 1024, gl::GL_COLOR_BUFFER_BIT, gl::GL_LINEAR);

		SDL_GL_SwapWindow(window);

		auto current_frame = std::chrono::high_resolution_clock::now();
		delta_time = (last_frame - current_frame).count() / 1000.f / 1000.f / 1000.f;
		last_frame = current_frame;
	}

	gl::glDeleteBuffers(1, &vb);

	gl::glDeleteVertexArrays(1, &vao);

	pp.reset();

	fs.reset();

	vs.reset();


	SDL_GL_DeleteContext(ctx);

	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
