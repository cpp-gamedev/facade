#pragma once
#include <facade/scene/id.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/texture.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <span>
#include <string>

namespace facade {
class Pipeline;

///
/// \brief Texture lookup for materials.
///
struct TextureStore {
	///
	/// \brief Textures identified by Id as index into this span.
	///
	std::span<Texture const> textures;
	///
	/// \brief White texture.
	///
	Texture const& white;

	///
	/// \brief Obtain a texture corresponding to index if present, else white.
	///
	Texture const& get(std::optional<std::size_t> index) const;
};

///
/// \brief Base Material: stores render parameters for a mesh.
///
class Material {
  public:
	///
	/// \brief Convert sRGB encoded colour to linear.
	///
	static glm::vec4 to_linear(glm::vec4 const& srgb);
	///
	/// \brief Convert linear encoded colour to sRGB.
	///
	static glm::vec4 to_srgb(glm::vec4 const& linear);

	inline static std::string const default_shader_id{"default"};

	virtual ~Material() = default;

	///
	/// \brief Obtain the ID for the shader used by this material.
	/// \returns Shader ID for this material instance
	///
	virtual std::string const& shader_id() const { return default_shader_id; }
	///
	/// \brief Write descriptor sets into the passed pipeline.
	/// \param pipeline The pipeline to obtain sets from and bind them to
	/// \param store Texture lookup store
	///
	virtual void write_sets(Pipeline& pipeline, TextureStore const& store) const = 0;
};

///
/// \brief Unlit Material.
///
class UnlitMaterial : public Material {
  public:
	inline static std::string const shader_id_v{"unlit"};

	///
	/// \brief Tint to pass to shader.
	///
	glm::vec4 tint{1.0f};
	///
	/// \brief Texture to bind over geometry. (White if not provided.)
	///
	std::optional<Id<Texture>> texture{};

	std::string const& shader_id() const override { return shader_id_v; }
	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};

///
/// \brief PBR lit material.
///
class LitMaterial : public Material {
  public:
	///
	/// \brief Base colour factor.
	///
	glm::vec3 albedo{1.0f};
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

	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};
} // namespace facade
