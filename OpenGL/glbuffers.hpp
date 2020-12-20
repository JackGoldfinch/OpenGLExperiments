#pragma once

struct gl_object {
	static gl::GLuint init() {

	}
};

template<gl::GLenum TYPE>
struct gl_buffer {
	static gl::GLuint create_buffer() {
		gl::GLuint buffer;
		gl::glCreateBuffers(1, &buffer);
		gl::glBindBuffer(TYPE, buffer);
		return buffer;
	}

	static void delete_buffer(gl::GLuint &buffer) {
		gl::glDeleteBuffers(1, &buffer);
		buffer = 0;
	}
};

struct data {};

template<class DATA>
struct uniform_buffer_object {
	static_assert(std::is_base_of_v<data, DATA>, "'DATA' class not derived from 'data'.");

	gl::GLuint buffer_ = 0;
	gl::GLuint binding_ = 0;

	DATA data_;

	static constexpr size_t DATA_SIZE = sizeof(DATA);

	uniform_buffer_object(gl::GLuint binding):
		binding_(binding),
		buffer_{ gl_buffer<gl::GL_UNIFORM_BUFFER>::create_buffer() } {
		gl::glBindBuffer(gl::GL_UNIFORM_BUFFER, buffer_);
		gl::glNamedBufferData(buffer_, DATA_SIZE, nullptr, gl::GL_STATIC_DRAW);
		gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, binding_, buffer_);
	}

	virtual ~uniform_buffer_object() {
		gl_buffer<gl::GL_UNIFORM_BUFFER>::delete_buffer(buffer_);
	}

	void update(gl::GLintptr offset, gl::GLsizeiptr size, const void *data) {
		gl::glNamedBufferSubData(buffer_, offset, size, data);
	}

	void update() {
		update(0, DATA_SIZE, &data_);
	}
};
