#include <facade/scene/scene.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/vk/texture.hpp>
#include <map>

namespace facade {
namespace {
struct Img1x1 {
	std::byte bytes[4]{};

	Image::View view() const {
		return Image::View{
			.bytes = {reinterpret_cast<std::byte const*>(bytes), std::size(bytes)},
			.extent = {1, 1},
		};
	}

	static Img1x1 make(std::array<std::uint8_t, 4> const& rgba) {
		return {
			.bytes =
				{
					static_cast<std::byte>(rgba[0]),
					static_cast<std::byte>(rgba[1]),
					static_cast<std::byte>(rgba[2]),
					static_cast<std::byte>(rgba[3]),
				},
		};
	}
};
} // namespace

struct Scene::TreeBuilder {
	Scene& out_scene;

	bool set_camera(TreeImpl& out_tree, Id<Node> id, std::span<Node const> nodes) const {
		auto& node = nodes[id];
		if (auto cam = node.find<Camera>()) {
			out_tree.camera = id;
			return true;
		}
		for (auto const child : node.children) {
			if (set_camera(out_tree, child, nodes)) { return true; }
		}
		return false;
	}

	void set_camera(TreeImpl& out_tree, std::span<Node const> nodes) const {
		for (auto const& id : out_tree.roots) {
			if (set_camera(out_tree, id, nodes)) { return; }
		}
		auto node = Node{.name = "camera"};
		node.attach<Camera>(0);
		auto const id = out_scene.m_storage.resources.nodes.size();
		out_scene.m_storage.resources.nodes.m_array.push_back(std::move(node));
		out_tree.roots.push_back(id);
		out_tree.cameras.push_back(id);
		out_tree.camera = out_tree.cameras.front();
	}

	TreeImpl operator()(Id<Tree> id) {
		auto ret = TreeImpl{.self = id};
		ret.roots = out_scene.m_storage.data.trees[id];
		set_camera(ret, out_scene.m_storage.resources.nodes.view());
		return ret;
	}
};

Scene::Scene(Gfx const& gfx) : m_gfx(gfx), m_sampler(gfx) { add_default_camera(); }

Id<Camera> Scene::add(Camera camera) {
	auto const id = m_storage.resources.cameras.size();
	m_storage.resources.cameras.m_array.push_back(std::move(camera));
	return id;
}

Id<Sampler> Scene::add(Sampler::CreateInfo const& create_info) {
	auto const id = m_storage.resources.samplers.size();
	m_storage.resources.samplers.m_array.emplace_back(m_gfx, create_info);
	return id;
}

Id<Material> Scene::add(Material material) {
	auto const id = m_storage.resources.materials.size();
	m_storage.resources.materials.m_array.push_back(std::move(material));
	return id;
}

Id<StaticMesh> Scene::add(Geometry::Packed const& geometry, std::string name) {
	auto const id = m_storage.resources.static_meshes.size();
	m_storage.resources.static_meshes.m_array.emplace_back(m_gfx, geometry, std::move(name));
	return id;
}

Id<Texture> Scene::add(Image::View image, Id<Sampler> sampler_id, ColourSpace colour_space) {
	auto sampler = [&] {
		if (sampler_id >= m_storage.resources.samplers.size()) {
			logger::warn("[Scene] Invalid sampler id: [{}], using default", sampler_id.value());
			return default_sampler();
		}
		return m_storage.resources.samplers[sampler_id].sampler();
	}();
	auto const ret = m_storage.resources.textures.size();
	m_storage.resources.textures.m_array.emplace_back(m_gfx, sampler, image, Texture::CreateInfo{.colour_space = colour_space});
	return ret;
}

Id<Mesh> Scene::add(Mesh mesh) {
	check(mesh);
	return add_unchecked(std::move(mesh));
}

Id<Node> Scene::add(Node node, std::optional<Id<Node>> parent) {
	check(node);
	auto& nodes = m_storage.resources.nodes;
	auto ret = nodes.size();
	node.self = ret;
	nodes.m_array.push_back(std::move(node));
	if (parent) {
		auto* p = nodes.find(*parent);
		if (!p) { throw Error{fmt::format("Scene {}: Invalid parent Node Id: {}", m_name, *parent)}; }
		p->children.push_back(ret);
	} else {
		m_tree.roots.push_back(ret);
	}
	return ret;
}

std::vector<Texture> Scene::replace(std::vector<Texture>&& textures) { return std::exchange(m_storage.resources.textures.m_array, std::move(textures)); }

std::vector<StaticMesh> Scene::replace(std::vector<StaticMesh>&& static_meshes) {
	return std::exchange(m_storage.resources.static_meshes.m_array, std::move(static_meshes));
}

std::vector<SkinnedMesh> Scene::replace(std::vector<SkinnedMesh>&& skinned_meshes) {
	return std::exchange(m_storage.resources.skinned_meshes.m_array, std::move(skinned_meshes));
}

bool Scene::load(Id<Tree> id) {
	if (id >= tree_count()) { return false; }
	return load_tree(id);
}

Node& Scene::camera() { return const_cast<Node&>(std::as_const(*this).camera()); }

Node const& Scene::camera() const {
	assert(m_tree.camera < m_storage.resources.nodes.size());
	return m_storage.resources.nodes[m_tree.camera];
}

bool Scene::select_camera(Id<Node> target) {
	for (auto const& id : m_tree.cameras) {
		if (id == target) {
			m_tree.camera = id;
			return true;
		}
	}
	return false;
}

Texture Scene::make_texture(Image::View image) const { return Texture{m_gfx, default_sampler(), image}; }

void Scene::tick(float dt) {
	for (auto& animation : m_storage.resources.animations.view()) { animation.update(m_storage.resources.nodes.view(), dt); }
}

Node Scene::make_camera_node(Id<Camera> id) const {
	assert(id < m_storage.resources.cameras.size());
	auto node = Node{};
	node.name = "camera";
	node.transform.set_position({0.0f, 0.0f, 5.0f});
	node.attach<Camera>(id);
	return node;
}

void Scene::add_default_camera() {
	m_tree.cameras.push_back(m_storage.resources.nodes.size());
	m_tree.roots.push_back(m_storage.resources.nodes.size());
	m_storage.resources.nodes.m_array.push_back(make_camera_node(add(Camera{.name = "default"})));
	m_tree.camera = m_tree.cameras.front();
}

bool Scene::load_tree(Id<Tree> id) {
	assert(id < m_storage.data.trees.size());
	m_tree = TreeBuilder{*this}(id);
	return true;
}

Id<Mesh> Scene::add_unchecked(Mesh mesh) {
	auto const id = m_storage.resources.meshes.size();
	m_storage.resources.meshes.m_array.push_back(std::move(mesh));
	return id;
}

void Scene::check(Mesh const& mesh) const noexcept(false) {
	for (auto const& primitive : mesh.primitives) {
		if (primitive.skinned_mesh) {
			if (*primitive.skinned_mesh >= m_storage.resources.skinned_meshes.size()) {
				throw Error{fmt::format("Scene {}: Invalid Skinned Mesh Id: {}", m_name, *primitive.skinned_mesh)};
			}
		} else if (primitive.static_mesh >= m_storage.resources.static_meshes.size()) {
			throw Error{fmt::format("Scene {}: Invalid Static Mesh Id: {}", m_name, primitive.static_mesh)};
		}
		if (primitive.material && primitive.material->value() >= m_storage.resources.materials.size()) {
			throw Error{fmt::format("Scene {}: Invalid Material Id: {}", m_name, *primitive.material)};
		}
	}
}

void Scene::check(Node const& node) const noexcept(false) {
	if (auto const mesh_id = node.find<Mesh>(); mesh_id && *mesh_id >= m_storage.resources.meshes.size()) {
		throw Error{fmt::format("Scene {}: Invalid mesh [{}] in node", m_name, *mesh_id)};
	}
	if (auto const camera_id = node.find<Camera>(); camera_id && *camera_id >= m_storage.resources.cameras.size()) {
		throw Error{fmt::format("Scene {}: Invalid camera [{}] in node", m_name, *camera_id)};
	}
	for (auto const id : node.children) {
		if (id >= m_storage.resources.nodes.size()) { throw Error{fmt::format("Scene {}: Invalid child [{}] in node", m_name, id)}; }
	}
}
} // namespace facade
