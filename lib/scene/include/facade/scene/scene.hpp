#pragma once
#include <facade/scene/camera.hpp>
#include <facade/scene/id.hpp>
#include <facade/scene/lights.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/node.hpp>
#include <facade/scene/node_data.hpp>
#include <facade/scene/transform.hpp>
#include <facade/util/enum_array.hpp>
#include <facade/util/image.hpp>
#include <facade/vk/buffer.hpp>
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace dj {
class Json;
}

namespace facade {
struct Mesh {
	struct Primitive {
		Id<StaticMesh> static_mesh{};
		std::optional<Id<Material>> material{};
	};

	std::vector<Primitive> primitives{};
};

struct DataProvider;

class Scene {
  public:
	static constexpr auto id_v = Id<Node>{0};

	explicit Scene(Gfx const& gfx);

	bool load_gltf(dj::Json const& root, DataProvider const& provider) noexcept(false);
	bool load_gltf(std::string_view path);

	Id<Camera> add(Camera camera);
	Id<Sampler> add(Sampler sampler);
	Id<Material> add(std::unique_ptr<Material> material);
	Id<StaticMesh> add(StaticMesh mesh);
	Id<Image> add(Image image);
	Id<Mesh> add(Mesh mesh);
	Id<Node> add(Node node, Id<Node> parent);

	Id<Scene> id() const { return m_tree.id; }
	std::size_t scene_count() const { return m_storage.data.trees.size(); }
	bool load(Id<Scene> id);

	Node const* find_node(Id<Node> id) const;
	Node* find_node(Id<Node> id);
	Material* find_material(Id<Material> id) const;
	Mesh const* find_mesh(Id<Mesh> id) const;
	std::span<Node> roots() { return m_tree.roots; }
	std::span<Node const> roots() const { return m_tree.roots; }

	std::size_t camera_count() const { return m_storage.cameras.size(); }
	bool select_camera(Id<Camera> id);
	Node& camera();
	Node const& camera() const;

	vk::Sampler default_sampler() const { return m_sampler.sampler(); }
	Texture make_texture(Image::View image) const;

	std::vector<DirLight> dir_lights{};

  private:
	struct TreeBuilder;

	struct Tree {
		struct Data {
			std::vector<std::size_t> roots{};
		};

		std::vector<Node> roots{};
		Id<Node> camera{};
		Id<Scene> id{};
	};

	struct Data {
		std::vector<NodeData> nodes{};
		std::vector<Tree::Data> trees{};
	};

	struct Storage {
		std::vector<Camera> cameras{};
		std::vector<Sampler> samplers{};
		std::vector<std::unique_ptr<Material>> materials{};
		std::vector<StaticMesh> static_meshes{};
		std::vector<Image> images{};
		std::vector<Texture> textures{};
		std::vector<Mesh> meshes{};
		std::vector<glm::mat4x4> instances{};
		Data data{};
		Id<Node> next_node{};
	};

	void add_default_camera();
	bool load_tree(Id<Scene> id);
	Id<Mesh> add_unchecked(Mesh mesh);
	Id<Node> add_unchecked(std::vector<Node>& out, Node&& node);
	static Node const* find_node(std::span<Node const> nodes, Id<Node> id);

	void check(Mesh const& mesh) const noexcept(false);
	void check(Node const& node) const noexcept(false);

	Gfx m_gfx;
	Sampler m_sampler;
	Storage m_storage{};
	std::string m_name{};
	Tree m_tree{};

	friend class SceneRenderer;
};
} // namespace facade
