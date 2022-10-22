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

struct TextureStore {
	std::span<Texture const> textures;
	Texture const& white;

	Texture const& get(std::optional<std::size_t> index) const;
};

class Material {
  public:
	inline static std::string const default_shader_id{"default"};

	virtual ~Material() = default;

	virtual std::string const& shader_id() const { return default_shader_id; }
	virtual void write_sets(Pipeline& pipeline, TextureStore const& store) const = 0;
};

class UnlitMaterial : public Material {
  public:
	inline static std::string const shader_id_v{"unlit"};

	glm::vec4 tint{1.0f};
	std::optional<Id<Texture>> texture{};

	std::string const& shader_id() const override { return shader_id_v; }
	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};

class LitMaterial : public Material {
  public:
	glm::vec3 albedo{1.0f};
	float metallic{0.5f};
	float roughness{0.5f};
	std::optional<Id<Texture>> base_colour{};
	std::optional<Id<Texture>> roughness_metallic{};

	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};
} // namespace facade
