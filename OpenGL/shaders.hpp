#pragma once

struct shader {
	const gl::GLenum type;

	gl::GLint id_;

	shader(gl::GLenum type):
	type(type) {}
};

template<gl::GLenum T>
struct shader_program : shader {
	shader_program(const std::filesystem::path &path):shader(T) {

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
					gl::GLint uniform_blocks_num;
					gl::glGetProgramiv(id_, gl::GL_ACTIVE_UNIFORM_BLOCKS, &uniform_blocks_num);

					gl::GLint uniform_blocks_len;
					gl::glGetProgramiv(id_, gl::GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &uniform_blocks_len);

					std::vector<char> uniform_blocks_buf(uniform_blocks_len);
					for (int i = 0; i < uniform_blocks_num; ++i) {
						gl::glGetActiveUniformBlockName(id_, i, uniform_blocks_len, nullptr, uniform_blocks_buf.data());
					}

					gl::GLint uniforms_num;
					gl::glGetProgramiv(id_, gl::GL_ACTIVE_UNIFORMS, &uniforms_num);

					gl::GLint uniforms_len;
					gl::glGetProgramiv(id_, gl::GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniforms_len);

					std::vector<char> uniforms_buf(uniforms_len);
					for (int i = 0; i < uniforms_num; ++i) {
						gl::glGetActiveUniformName(id_, i, uniforms_len, nullptr, uniforms_buf.data());
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
};

const std::unordered_map<gl::GLenum, gl::UseProgramStageMask> program_stage_mask{
	{gl::GL_VERTEX_SHADER, gl::GL_VERTEX_SHADER_BIT},
	{gl::GL_FRAGMENT_SHADER, gl::GL_FRAGMENT_SHADER_BIT}
};

struct shader_pipeline {
	using init_list = std::initializer_list<const shader *>;

	gl::GLuint id_;

	shader_pipeline(const init_list &programs) {
		gl::glCreateProgramPipelines(1, &id_);

		for (auto program : programs) {
			try {
				gl::glUseProgramStages(id_, program_stage_mask.at(program->type), program->id_);
			} catch (...) {}
		}

		gl::glUseProgram(0);
	}

	~shader_pipeline() {
		gl::glDeleteProgramPipelines(1, &id_);
	}

	void bind() const {
		gl::glBindProgramPipeline(id_);
	}
};
