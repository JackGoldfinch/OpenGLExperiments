#pragma once

struct gl_texture : gl_object {
	virtual void bind() = 0;
	virtual void unbind() = 0;
};


template<gl::GLenum target> constexpr bool is_array = false;
template<> constexpr bool is_array<gl::GL_TEXTURE_1D_ARRAY> = true;
template<> constexpr bool is_array<gl::GL_TEXTURE_2D_ARRAY> = true;
template<> constexpr bool is_array<gl::GL_TEXTURE_CUBE_MAP_ARRAY> = true;
template<> constexpr bool is_array<gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY> = true;

template<gl::GLenum target> constexpr bool is_multisampled = false;
template<> constexpr bool is_multisampled<gl::GL_TEXTURE_2D_MULTISAMPLE> = true;
template<> constexpr bool is_multisampled<gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY> = true;


template<gl::GLenum target>
struct texture : gl_texture {
	static texture *bound_;

	texture() {
		gl::glCreateTextures(target, 1, &id_);
		bind();
		setup_filtering();
	}

	virtual ~texture() {
		gl::glDeleteTextures(1, &id_);
	}

	void setup_filtering() {
		gl::glTexParameteri(target, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
		gl::glTexParameteri(target, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
	}

	void bind() {
		if (bound_ != this) {
			bound_ = this;
			gl::glBindTexture(target, id_);
		}
	}

	void unbind() {
		if (bound_ == this) {
			bound_ = nullptr;
			gl::glBindTexture(target, 0);
		}
	}
};

template<gl::GLenum target>
texture<target> *texture<target>::bound_ = nullptr;


struct texture_1d : texture<gl::GL_TEXTURE_1D> {
	void image(gl::GLint level, gl::GLenum internal_format, gl::GLsizei width, gl::GLenum format, gl::GLenum type, const void *pixels) {
		gl::glTexImage1D(gl::GL_TEXTURE_1D, level, internal_format, width, 0, format, type, pixels);
	}
};

struct texture_2d : texture<gl::GL_TEXTURE_2D> {
	void image(gl::GLint level, gl::GLenum internal_format, gl::GLsizei width, gl::GLsizei height, gl::GLenum format, gl::GLenum type, const void *pixels) {
		gl::glTexImage2D(gl::GL_TEXTURE_2D, level, internal_format, width, height, 0, format, type, pixels);
	}
};

struct texture_2d_multisample : texture<gl::GL_TEXTURE_2D_MULTISAMPLE> {
	void image(gl::GLsizei samples, gl::GLenum internal_format, gl::GLsizei width, gl::GLsizei height, gl::GLboolean fixed_sample_locations) {
		gl::glTexImage2DMultisample(gl::GL_TEXTURE_2D_MULTISAMPLE, samples, internal_format, width, height, fixed_sample_locations);
	}
};
