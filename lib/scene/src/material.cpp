#include <facade/scene/material.hpp>
#include <facade/vk/pipeline.hpp>
#include <glm/gtc/color_space.hpp>

namespace facade {
namespace {
struct Mat {
	glm::vec4 albedo;
	glm::vec4 m_r_aco_am;
};

// std::bit_cast not available on GCC 10.x
float bit_cast_f(Material::AlphaMode const mode) {
	auto ret = float{};
	std::memcpy(&ret, &mode, sizeof(mode));
	return ret;
}
} // namespace

Texture const& TextureStore::get(std::optional<std::size_t> const index) const {
	if (!index || *index >= textures.size()) { return white; }
	return textures[*index];
}

glm::vec4 Material::to_linear(glm::vec4 const& srgb) { return glm::convertSRGBToLinear(srgb); }
glm::vec4 Material::to_srgb(glm::vec4 const& linear) { return glm::convertLinearToSRGB(linear); }

void UnlitMaterial::write_sets(Pipeline& pipeline, TextureStore const& store) const {
	auto& set1 = pipeline.next_set(1);
	set1.update(0, store.get(texture).descriptor_image());
	auto& set2 = pipeline.next_set(2);
	auto const mat = Mat{
		.albedo = to_linear(tint),
		.m_r_aco_am = {},
	};
	set2.write(0, mat);
	pipeline.bind(set1);
	pipeline.bind(set2);
}

void LitMaterial::write_sets(Pipeline& pipeline, TextureStore const& store) const {
	auto& set1 = pipeline.next_set(1);
	set1.update(0, store.get(base_colour).descriptor_image());
	set1.update(1, store.get(roughness_metallic).descriptor_image());
	auto& set2 = pipeline.next_set(2);
	auto const mat = Mat{
		.albedo = to_linear({albedo, 1.0f}),
		.m_r_aco_am = {metallic, roughness, alpha_cutoff, bit_cast_f(alpha_mode)},
	};
	set2.write(0, mat);
	pipeline.bind(set1);
	pipeline.bind(set2);
}
} // namespace facade
