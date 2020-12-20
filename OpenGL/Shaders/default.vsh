#version 460

layout(location = 0) in vec2 pos;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection;
	mat4 view;
	mat4 model;
} matrices;

out gl_PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
};

void main() {
	gl_Position = matrices.projection * matrices.model * vec4(pos.x, pos.y, 0.0, 1.0);
}