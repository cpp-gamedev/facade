#pragma once
#include <facade/scene/id.hpp>
#include <facade/util/colour_space.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <span>
#include <string_view>

namespace facade {
class Texture;
class Pipeline;

struct TextureProvider {
	virtual Texture const* get(std::size_t index, ColourSpace colour_space) const = 0;
};

struct TextureStore {
	Texture const& white;
	TextureProvider const& provider;

	Texture const& get(std::optional<std::size_t> index, ColourSpace colour_space) const;
};

class Material {
  public:
	static constexpr std::string_view default_shader_id{"default"};

	virtual ~Material() = default;

	virtual std::string_view shader_id() const { return default_shader_id; }
	virtual void write_sets(Pipeline& pipeline, TextureStore const& store) const = 0;
};

class UnlitMaterial : public Material {
  public:
	static constexpr std::string_view shader_id_v{"unlit"};

	glm::vec4 tint{1.0f};
	std::optional<Id<Texture>> texture{};

	std::string_view shader_id() const override { return shader_id_v; }
	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};

class TestMaterial : public Material {
  public:
	glm::vec3 albedo{1.0f};
	float metallic{0.5f};
	float roughness{0.5f};
	std::optional<Id<Texture>> base_colour{};
	std::optional<Id<Texture>> roughness_metallic{};

	void write_sets(Pipeline& pipeline, TextureStore const& store) const override;
};
} // namespace facade
