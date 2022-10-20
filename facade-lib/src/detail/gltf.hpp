#pragma once
#include <facade/scene/transform.hpp>
#include <facade/util/byte_buffer.hpp>
#include <facade/util/geometry.hpp>
#include <facade/util/image.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <string>
#include <vector>

namespace dj {
class Json;
}

namespace facade {
struct DataProvider;

namespace gltf {
struct TextureInfo {
	std::size_t texture{};
	std::size_t tex_coord{};
};

struct NormalTextureInfo {
	TextureInfo info{};
	float scale{1.0f};
};

struct OccusionTextureInfo {
	TextureInfo info{};
	float strength{1.0f};
};

struct PbrMetallicRoughness {
	std::optional<TextureInfo> base_colour_texture{};
	std::optional<TextureInfo> metallic_roughness_texture{};
	glm::vec4 base_colour_factor{1.0f};
	float metallic_factor{1.0f};
	float roughness_factor{1.0f};
};

struct Material {
	enum class AlphaMode { eOpaque, eMask, eBlend };

	std::string name{};
	PbrMetallicRoughness pbr{};
	std::optional<NormalTextureInfo> normal_texture{};
	std::optional<OccusionTextureInfo> occlusion_texture{};
	std::optional<TextureInfo> emissive_texture{};
	glm::vec3 emissive_factor{};
	AlphaMode alpha_mode{};
	float alpha_cutoff{0.5f};
	bool double_sided{};
};

struct Mesh {
	struct Primitive {
		std::size_t geometry{};
		std::optional<std::size_t> material{};
	};

	std::string name{};
	std::vector<Primitive> primitives{};
};

struct Camera {
	enum class Type { ePerspective, eOrthographic };

	struct Perspective {
		float yfov{};
		float znear{};

		float aspect_ratio{};
		std::optional<float> zfar{};
	};

	struct Orthographic {
		float xmag{};
		float ymag{};
		float zfar{};
		float znear{};
	};

	std::string name{};
	Perspective perspective{};
	Orthographic orthographic{};
	Type type{};
};

struct Node {
	enum class Type { eNone, eMesh, eCamera };

	Transform transform{};
	std::vector<std::size_t> children{};
	std::size_t index{};
	Type type{};
};

struct Scene {
	std::vector<std::size_t> root_nodes{};
};

struct Asset {
	std::vector<Camera> cameras{};
	std::vector<Geometry> geometries{};
	std::vector<Image> images{};
	std::vector<Material> materials{};
	std::vector<Mesh> meshes{};
	std::vector<Node> nodes{};

	std::vector<Scene> scenes{};
	std::size_t start_scene{};

	static Asset parse(dj::Json const& json, DataProvider const& provider);
};
} // namespace gltf
} // namespace facade
