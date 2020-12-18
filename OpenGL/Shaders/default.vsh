#version 460

layout(location = 0) in vec2 pos;

layout(std140) uniform Matrices{
	mat4 model;
	mat4 projection;
} matrices;

uniform mat4 model;
uniform mat4 projection;

out gl_PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
};

void main() {
	gl_Position = projection * matrices.model * vec4(pos.x, pos.y, 0.0, 1.0);
}