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
	/// \brief Black texture;
	///
	Texture const& black;

	///
	/// \brief Obtain a texture corresponding to index if present, else fallback.
	///
	Texture const& get(std::optional<std::size_t> index, Texture const& fallback) const;
	///
	/// \brief Obtain a texture corresponding to index if present, else white.
	///
	Texture const& get(std::optional<std::size_t> index) const { return get(index, white); }
};

///
/// \brief Base Material: stores render parameters for a mesh.
///
class MaterialBase {
  public:
	///
	/// \brief The alpha blend mode.
	///
	enum class AlphaMode : std::uint32_t { eOpaque = 0, eBlend, eMask };

	inline static std::string const default_shader_id{"default"};

	virtual ~MaterialBase() = default;

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

	std::string name{"(Unnamed)"};
};

class Material {
  public:
	using AlphaMode = MaterialBase::AlphaMode;

	Material(std::unique_ptr<MaterialBase>&& base) : m_base(std::move(base)) {}

	std::string const& shader_id() const {
		assert(m_base);
		return m_base->shader_id();
	}

	void write_sets(Pipeline& pipeline, TextureStore const& store) const {
		assert(m_base);
		m_base->write_sets(pipeline, store);
	}

	std::string_view name() const { return m_base->name; }

	MaterialBase& base() const {
		assert(m_base);
		return *m_base;
	}

  private:
	std::unique_ptr<MaterialBase> m_base;
};

///
/// \brief Unlit Material.
///
class UnlitMaterial : public MaterialBase {
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
class LitMaterial : public MaterialBase {
  public:
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

	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};
} // namespace facade
