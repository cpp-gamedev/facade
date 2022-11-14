#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/lights.hpp>
#include <facade/scene/node.hpp>
#include <facade/scene/node_data.hpp>
#include <facade/scene/scene_resources.hpp>
#include <facade/util/image.hpp>
#include <facade/util/ptr.hpp>
#include <facade/util/transform.hpp>
#include <facade/vk/pipeline.hpp>
#include <memory>
#include <span>
#include <vector>

namespace facade {
struct DataProvider;

///
/// \brief Models a 3D scene.
///
/// A single Scene holds all the data in a GLTF asset, which can contain *multiple* GLTF scenes.
/// All the GLTF scenes are stored and available for use: only one is active at a time.
///
class Scene {
  public:
	using Resources = SceneResources;

	// defined in loader.hpp
	class GltfLoader;

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
	Id<Material> add(Material material);
	///
	/// \brief Add a StaticMesh.
	/// \param geometry Geometry to initialize StaticMesh with
	/// \returns Id to stored StaticMesh
	///
	Id<StaticMesh> add(Geometry const& geometry, std::string name = "(Unnamed)");
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
	/// \brief Replace all textures in resources.
	/// \param textures Texture instances to swap in
	/// \returns Previously stored textures
	///
	/// Intended for loaders.
	///
	std::vector<Texture> replace(std::vector<Texture>&& textures);
	///
	/// \brief Replace all static meshes in resources.
	/// \param static_meshes StaticMesh instances to swap in
	/// \returns Previously stored static meshes
	///
	/// Intended for loaders.
	///
	std::vector<StaticMesh> replace(std::vector<StaticMesh>&& static_meshes);

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
	std::size_t camera_count() const { return m_storage.resources.cameras.size(); }
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
	/// \brief Obtain a mutable reference to the scene's resources.
	/// \returns Mutable reference to SceneResources
	///
	Resources& resources() { return m_storage.resources; }
	///
	/// \brief Obtain an immutable reference to the scene's resources.
	/// \returns Immutable reference to SceneResources
	///
	Resources const& resources() const { return m_storage.resources; }

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
		Resources resources{};
		Data data{};
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
