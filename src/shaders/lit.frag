#version 450 core

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
};

struct Material {
	vec4 albedo;
	vec4 metallic_roughness;
};

layout (set = 0, binding = 1) readonly buffer DL {
	DirLight dir_lights[];
};

layout (set = 1, binding = 0) uniform sampler2D base_colour;
layout (set = 1, binding = 1) uniform sampler2D roughness_metallic;

layout (set = 2, binding = 0) uniform M {
	Material material;
};

layout (location = 0) in vec3 in_rgb;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_pos_frag;
layout (location = 4) in vec3 in_pos_view;

layout (location = 0) out vec4 out_rgba;

const float pi_v = 3.14159;

float distribution_ggx(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = pi_v * denom * denom;

	return num / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float geometry_smith(float NdotV, float NdotL, float roughness) {
	float ggx2 = geometry_schlick_ggx(NdotV, roughness);
	float ggx1 = geometry_schlick_ggx(NdotL, roughness);
	return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos, vec3 F0) { return F0 + (vec3(1.0) - F0) * pow(max(1.0 - cos, 0.0), 5.0); }

vec3 gamc(vec3 a) {
	float exp = 1.0f / 2.2f;
	return vec3(
		pow(a.x, exp),
		pow(a.y, exp),
		pow(a.z, exp)
	);
}

vec3 cook_torrance() {
	float roughness = material.metallic_roughness.y * texture(roughness_metallic, in_uv).g;
	float metallic = material.metallic_roughness.x * texture(roughness_metallic, in_uv).b;
	vec3 f0 = mix(vec3(0.04), vec3(material.albedo), metallic);

	vec3 L0 = vec3(0.0);
	vec3 V = normalize(in_pos_view - in_pos_frag);
	vec3 N = in_normal;
	for (int i = 0; i < dir_lights.length(); ++i) {
		DirLight light = dir_lights[i];
		vec3 L = -light.direction;
		vec3 H = normalize(V + L);

		float NdotL = max(dot(N, L), 0.0);
		float NdotV = max(dot(N, V), 0.0);

		float NDF = distribution_ggx(N, H, roughness);
		float G = geometry_smith(NdotV, NdotL, roughness);
		vec3 F = fresnel_schlick(max(dot(H, V), 0.0), f0);

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		vec3 num = NDF * kS * G;
		float denom = 4.0 * NdotV * NdotL + 0.0001;
		vec3 spec = num / denom;

		L0 += (kD * vec3(material.albedo) / pi_v + spec) * light.diffuse * NdotL;
	}

	vec3 colour = L0;

	colour /= (colour + vec3(1.0));
	return colour;
}

void main() {
	out_rgba = vec4(cook_torrance(), 1.0) * texture(base_colour, in_uv);
}
