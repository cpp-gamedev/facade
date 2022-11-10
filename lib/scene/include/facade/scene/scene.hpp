#pragma once
#include <facade/scene/camera.hpp>
#include <facade/scene/id.hpp>
#include <facade/scene/lights.hpp>
#include <facade/scene/load_status.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/mesh.hpp>
#include <facade/scene/node.hpp>
#include <facade/scene/node_data.hpp>
#include <facade/util/enum_array.hpp>
#include <facade/util/image.hpp>
#include <facade/util/ptr.hpp>
#include <facade/util/transform.hpp>
#include <facade/vk/buffer.hpp>
#include <facade/vk/pipeline.hpp>
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace dj {
class Json;
}

namespace facade {
struct DataProvider;

///
/// \brief Concurrent status for multi-threaded loading
///
struct AtomicLoadStatus {
	std::atomic<LoadStage> stage{};
	std::atomic<std::size_t> total{};
	std::atomic<std::size_t> done{};

	void reset() {
		stage = LoadStage::eNone;
		total = {};
		done = {};
	}
};

///
/// \brief Immutable view of the resources stored in the scene.
///
struct SceneResources {
	std::span<Camera const> cameras{};
	std::span<Sampler const> samplers{};
	std::span<std::unique_ptr<Material> const> materials{};
	std::span<StaticMesh const> static_meshes{};
	std::span<Texture const> textures{};
	std::span<Mesh const> meshes{};
};

///
/// \brief Models a 3D scene.
///
/// A single Scene holds all the data in a GLTF asset, which can contain *multiple* GLTF scenes.
/// All the GLTF scenes are stored and available for use: only one is active at a time.
///
class Scene {
  public:
	using Resources = SceneResources;

	///
	/// \brief Represents a single GTLF scene.
	///
	struct Tree {};

	///
	/// \brief "Null" Id for a Node: refers to no Node.
	///
	/// Used as the parent Id for root nodes.
	///
	static constexpr auto null_id_v = Id<Node>{0};

	///
	/// \brief Construct a Scene.
	/// \param gfx The graphics context (internal)
	///
	/// An active Gfx instance is not directly accessible; Engine owns any Scenes in use and offers APIs to access / modify / async-load them.
	///
	explicit Scene(Gfx const& gfx);

	///
	/// \brief Load data from a GLTF file.
	/// \param root The root JSON node for the GLTF asset
	/// \param provider Data provider with the JSON parent directory mounted
	/// \param out_status Optional pointer to AtomicLoadStatus to be updated by the Scene
	/// \returns true If successfully loaded
	///
	/// If the GLTF data fails to load, the scene data will remain unchanged.
	/// This function purposely throws on fatal errors.
	///
	bool load_gltf(dj::Json const& root, DataProvider const& provider, AtomicLoadStatus* out_status = {}) noexcept(false);

	///
	/// \brief Add a Camera.
	/// \param camera Camera instance to add
	/// \returns Id to stored Camera
	///
	Id<Camera> add(Camera camera);
	///
	/// \brief Add a Sampler.
	/// \param create_info Initialization data for the Sampler to create
	/// \returns Id to stored Sampler
	///
	Id<Sampler> add(Sampler::CreateInfo const& create_info);
	///
	/// \brief Add a Material.
	/// \param material Concrete Material instance to add
	/// \returns Id to stored Material
	///
	Id<Material> add(std::unique_ptr<Material> material);
	///
	/// \brief Add a StaticMesh.
	/// \param geometry Geometry to initialize StaticMesh with
	/// \returns Id to stored StaticMesh
	///
	Id<StaticMesh> add(Geometry const& geometry, std::string name = "(unnamed)");
	///
	/// \brief Add a Texture.
	/// \param image Image to use for the Texture
	/// \param sampler Id to the sampler to use for the Texture
	/// \param colour_space Whether to image is encoded as sRGB or linear
	/// \returns Id to stored Texture
	///
	Id<Texture> add(Image::View image, Id<Sampler> sampler, ColourSpace colour_space = ColourSpace::eSrgb);
	///
	/// \brief Add a Mesh.
	/// \param mesh Mesh instance to add
	/// \returns Id to stored Mesh
	///
	Id<Mesh> add(Mesh mesh);
	///
	/// \brief Add a Node.
	/// \param node Node instance to add
	/// \returns Id to stored Node
	///
	Id<Node> add(Node node, Id<Node> parent);

	///
	/// \brief Obtain the current Tree Id.
	/// \returns Current Tree Id
	///
	Id<Tree> tree_id() const { return m_tree.id; }
	///
	/// \brief Obtain the number of stored Trees.
	/// \returns Number of stored trees
	///
	std::size_t tree_count() const { return m_storage.data.trees.size(); }
	///
	/// \brief Load a Tree by its Id.
	/// \param id Tree Id to load
	/// \returns false If Id is invalid (out of bounds)
	///
	bool load(Id<Tree> id);

	///
	/// \brief Obtain an immutable pointer to the Node corresponding to its Id.
	/// \param id Id of the Node to search for
	/// \returns nullptr if Node with id wasn't found
	///
	Ptr<Node const> find(Id<Node> id) const;
	///
	/// \brief Obtain a mutable pointer to the Node corresponding to its Id.
	/// \param id Id of the Node to search for
	/// \returns nullptr if Node with id wasn't found
	///
	Ptr<Node> find(Id<Node> id);
	///
	/// \brief Obtain a mutable pointer to the Material corresponding to its Id.
	/// \param id Id of the Material
	/// \returns nullptr if id is invalid (out of bounds)
	///
	Ptr<Material> find(Id<Material> id) const;
	///
	/// \brief Obtain an immutable pointer to the StaticMesh corresponding to its Id.
	/// \param id Id of the StaticMesh
	/// \returns nullptr if id is invalid (out of bounds)
	///
	Ptr<StaticMesh const> find(Id<StaticMesh> id) const;
	///
	/// \brief Obtain an immutable pointer to the Texture corresponding to its Id.
	/// \param id Id of the Texture
	/// \returns nullptr if id is invalid (out of bounds)
	///
	Ptr<Texture const> find(Id<Texture> id) const;
	///
	/// \brief Obtain an immutable pointer to the Mesh corresponding to its Id.
	/// \param id Id of the Mesh
	/// \returns nullptr if id is invalid (out of bounds)
	///
	Ptr<Mesh const> find(Id<Mesh> id) const;

	///
	/// \brief Obtain a mutable view into all the root nodes attached to the active Tree.
	/// \returns Mutable view into the root nodes attached to the active Tree
	///
	std::span<Node> roots() { return m_tree.roots; }
	///
	/// \brief Obtain an immutable view into all the root nodes attached to the active Tree.
	/// \returns Immutable view into the root nodes attached to the active Tree
	///
	std::span<Node const> roots() const { return m_tree.roots; }

	///
	/// \brief Obtain the total count of stored cameras.
	/// \returns Count of stored cameras (at least 1)
	///
	std::size_t camera_count() const { return m_storage.cameras.size(); }
	///
	/// \brief Select a Camera by its Id.
	/// \param id Id of Camera to select
	/// \returns false If id is invalid (out of bounds)
	///
	bool select(Id<Camera> id);
	///
	/// \brief Obtain a mutable reference to the Node containing the active Camera.
	///
	Node& camera();
	///
	/// \brief Obtain an immutable reference to the Node containing the active Camera.
	///
	Node const& camera() const;

	///
	/// \brief Obtain the default Sampler.
	///
	vk::Sampler default_sampler() const { return m_sampler.sampler(); }
	///
	/// \brief Make a Texture
	/// \param image Image::View to use for the Texture
	///
	/// Uses the default Sampler
	///
	Texture make_texture(Image::View image) const;

	///
	/// \brief Obtain a view into the stored resources.
	/// \returns A Resources object
	///
	Resources resources() const { return m_storage.resources(); }

	///
	/// \brief All the lights in the scene.
	///
	Lights lights{};

	///
	/// \brief Global pipeline state.
	///
	Pipeline::State pipeline_state{};

  private:
	struct TreeBuilder;

	struct TreeImpl {
		struct Data {
			std::vector<std::size_t> roots{};
		};

		std::vector<Node> roots{};
		Id<Node> camera{};
		Id<Tree> id{};
	};

	struct Data {
		std::vector<NodeData> nodes{};
		std::vector<TreeImpl::Data> trees{};
	};

	struct Storage {
		std::vector<Camera> cameras{};
		std::vector<Sampler> samplers{};
		std::vector<std::unique_ptr<Material>> materials{};
		std::vector<StaticMesh> static_meshes{};
		std::vector<Texture> textures{};
		std::vector<Mesh> meshes{};

		Data data{};

		Resources resources() const { return {cameras, samplers, materials, static_meshes, textures, meshes}; }
	};

	void add_default_camera();
	bool load_tree(Id<Tree> id);
	Id<Mesh> add_unchecked(Mesh mesh);
	Id<Node> add_unchecked(std::vector<Node>& out, Node&& node);
	static Node const* find_node(std::span<Node const> nodes, Id<Node> id);

	void check(Mesh const& mesh) const noexcept(false);
	void check(Node const& node) const noexcept(false);

	inline static std::atomic<Id<Node>::id_type> s_next_node{};

	Gfx m_gfx;
	Sampler m_sampler;
	Storage m_storage{};
	std::string m_name{};
	TreeImpl m_tree{};
};
} // namespace facade
