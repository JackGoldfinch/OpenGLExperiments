#version 460

layout(location = 0) in vec2 pos;

uniform mat4 transform;

out gl_PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
};

void main() {
	gl_Position = transform * vec4(pos.x, pos.y, 0.0, 1.0);
}