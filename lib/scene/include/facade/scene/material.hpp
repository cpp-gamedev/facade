#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/resource_array.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/texture.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <span>
#include <string>
#include <variant>

namespace facade {
class Pipeline;

///
/// \brief Alpha blend mode.
///
enum class AlphaMode : std::uint32_t { eOpaque = 0, eBlend, eMask };

///
/// \brief Texture lookup for materials.
///
struct TextureStore {
	///
	/// \brief Textures resource array.
	///
	ResourceArray<Texture> const& textures;
	///
	/// \brief White texture.
	///
	Texture const& white;
	///
	/// \brief Black texture;
	///
	Texture const& black;

	///
	/// \brief Obtain a texture corresponding to index if present, else fallback.
	///
	Texture const& get(std::optional<Id<Texture>> index, Texture const& fallback) const;
	///
	/// \brief Obtain a texture corresponding to index if present, else white.
	///
	Texture const& get(std::optional<Id<Texture>> index) const { return get(index, white); }
};

///
/// \brief Unlit Material.
///
struct UnlitMaterial {
	///
	/// \brief Tint to pass to shader.
	///
	glm::vec4 tint{1.0f};
	///
	/// \brief Texture to bind over geometry. (White if not provided.)
	///
	std::optional<Id<Texture>> texture{};

	void write_sets(Pipeline& pipeline, TextureStore const& store) const;
};

///
/// \brief PBR lit material.
///
struct LitMaterial {
	///
	/// \brief Base colour factor.
	///
	glm::vec3 albedo{1.0f};
	///
	/// \brief Emissive factor.
	///
	glm::vec3 emissive_factor{0.0f};
	///
	/// \brief Metallic factor.
	///
	float metallic{0.5f};
	///
	/// \brief Roughess factor.
	///
	float roughness{0.5f};
	///
	/// \brief Base colour texture.
	///
	std::optional<Id<Texture>> base_colour{};
	///
	/// \brief Roughness-metallic texture.
	///
	std::optional<Id<Texture>> roughness_metallic{};
	///
	/// \brief Emissive texture.
	///
	std::optional<Id<Texture>> emissive{};

	float alpha_cutoff{};
	AlphaMode alpha_mode{AlphaMode::eOpaque};

	void write_sets(Pipeline& pipeline, TextureStore const& store) const;
};

struct Material {
	std::variant<LitMaterial, UnlitMaterial> instance{};
	std::string name{"(Unnamed)"};

	void write_sets(Pipeline& pipeline, TextureStore const& store) const {
		std::visit([&pipeline, &store](auto const& mat) { mat.write_sets(pipeline, store); }, instance);
	}
};
} // namespace facade
