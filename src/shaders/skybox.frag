#version 450 core

layout (set = 1, binding = 0) uniform samplerCube cubemap;

layout (location = 0) in vec3 in_rgb;
layout (location = 3) in vec3 in_fpos;
layout (location = 4) in vec4 in_vpos_exposure;

layout (location = 0) out vec4 out_rgba;

// unused
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

void main() {
	out_rgba = vec4(in_rgb, 1.0) * texture(cubemap, in_fpos - in_vpos_exposure.xyz);
}
