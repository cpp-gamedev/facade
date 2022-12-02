#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/lights.hpp>
#include <facade/scene/node_data.hpp>
#include <facade/scene/scene_resources.hpp>
#include <facade/util/image.hpp>
#include <facade/util/ptr.hpp>
#include <facade/util/transform.hpp>
#include <memory>
#include <span>
#include <vector>

namespace facade {
struct DataProvider;

///
/// \brief Polygon rendering mode: applied scene-wide.
///
struct RenderMode {
	enum class Type { eFill, eWireframe };

	Type type{Type::eFill};
	float line_width{1.0f};

	bool operator==(RenderMode const&) const = default;
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

	// defined in loader.hpp
	class GltfLoader;

	///
	/// \brief Represents a single GTLF scene.
	///
	struct Tree {};

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
	// TODO: Overload for interleaved
	Id<StaticMesh> add(Geometry::Packed const& geometry, std::string name = "(Unnamed)");
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
	/// \param parent Node to parent to (if any)
	/// \returns Id to stored Node
	///
	Id<Node> add(Node node, std::optional<Id<Node>> parent = std::nullopt);

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
	/// \brief Replace all skinned meshes in resources.
	/// \param static_meshes SkinnedMesh instances to swap in
	/// \returns Previously stored skinned meshes
	///
	/// Intended for loaders.
	///
	std::vector<SkinnedMesh> replace(std::vector<SkinnedMesh>&& skinned_meshes);

	///
	/// \brief Obtain the current Tree Id.
	/// \returns Current Tree Id
	///
	Id<Tree> tree_id() const { return m_tree.self; }
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
	/// \brief Obtain the Ids of all the root nodes attached to the active Tree.
	/// \returns Mutable view into the root nodes attached to the active Tree
	///
	std::span<Id<Node> const> roots() const { return m_tree.roots; }

	///
	/// \brief Obtain the total count of stored cameras.
	/// \returns Count of stored cameras (at least 1)
	///
	std::size_t camera_count() const { return m_storage.resources.cameras.size(); }
	///
	/// \brief Obtain a mutable reference to the Node containing the active Camera.
	///
	Node& camera();
	///
	/// \brief Obtain an immutable reference to the Node containing the active Camera.
	///
	Node const& camera() const;
	///
	/// \brief Obtain all the Ids of all nodes that have cameras attached.
	///
	std::span<Id<Node> const> cameras() const { return m_tree.cameras; }
	///
	/// \brief Select the target node as the camera.
	///
	bool select_camera(Id<Node> target);

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
	/// \brief Render mode.
	///
	RenderMode render_mode{};

	///
	/// \brief Update animations and corresponding nodes.
	/// \param dt Duration to advance simulation by (time since last call to tick)
	///
	void tick(float dt);

  private:
	struct TreeBuilder;

	struct TreeImpl {
		std::vector<Id<Node>> roots{};
		std::vector<Id<Node>> cameras{};
		Id<Node> camera{};
		Id<Tree> self{};
	};

	struct Data {
		using Roots = std::vector<Id<Node>>;

		std::vector<NodeData> nodes{};
		std::vector<Roots> trees{};
	};

	struct Storage {
		Resources resources{};
		Data data{};
	};

	Node make_camera_node(Id<Camera> id) const;
	void add_default_camera();
	bool load_tree(Id<Tree> id);
	Id<Mesh> add_unchecked(Mesh mesh);

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
