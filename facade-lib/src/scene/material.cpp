#include <facade/scene/material.hpp>
#include <facade/vk/pipeline.hpp>
#include <facade/vk/texture.hpp>
#include <glm/gtc/color_space.hpp>

namespace facade {
namespace {
struct Mat {
	glm::vec4 albedo;
	glm::vec4 metallic_roughness;
};

glm::vec4 uncorrect(glm::vec4 const& in) { return glm::convertSRGBToLinear(in); }
} // namespace

Texture const& TextureStore::get(std::optional<std::size_t> const index) const {
	if (!index || *index >= textures.size()) { return white; }
	return textures[*index];
}

void UnlitMaterial::write_sets(Pipeline& pipeline, TextureStore const& store) const {
	auto& set1 = pipeline.next_set(1);
	set1.update(0, store.get(texture).descriptor_image());
	auto& set2 = pipeline.next_set(2);
	auto const mat = Mat{
		.albedo = uncorrect(tint),
		.metallic_roughness = {},
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
		.albedo = uncorrect({albedo, 1.0f}),
		.metallic_roughness = {metallic, roughness, 0.0f, 0.0f},
	};
	set2.write(0, mat);
	pipeline.bind(set1);
	pipeline.bind(set2);
}
} // namespace facade
