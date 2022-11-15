#include <facade/scene/scene.hpp>
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
	std::span<NodeData const> in_gnodes;

	void add(TreeImpl& out, NodeData const& child, std::vector<Node>& out_children) {
		auto node = Node{};
		node.name = child.name;
		node.transform = child.transform;
		bool set_cam{};
		switch (child.type) {
		case NodeData::Type::eMesh: node.attach(Id<Mesh>{child.index}); break;
		case NodeData::Type::eCamera: {
			node.attach(Id<Camera>{child.index});
			set_cam = true;
			break;
		}
		default: break;
		}
		for (auto const index : child.children) {
			auto& child = in_gnodes[index];
			add(out, child, node.m_children);
		}
		out_scene.check(node);
		auto const node_id = out_scene.add_unchecked(out_children, std::move(node));
		if (set_cam) { out.cameras.push_back(node_id); }
	}

	TreeImpl operator()(TreeImpl::Data const& tree, Id<Tree> id) {
		auto ret = TreeImpl{.id = id};
		for (auto const index : tree.roots) {
			assert(index < in_gnodes.size());
			add(ret, in_gnodes[index], ret.roots);
		}
		if (ret.cameras.empty()) {
			// add node with default camera
			ret.cameras.push_back(out_scene.add_unchecked(ret.roots, out_scene.make_camera_node(0)));
		}
		ret.camera = ret.cameras.front();
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

Id<StaticMesh> Scene::add(Geometry const& geometry, std::string name) {
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

Id<Node> Scene::add(Node node, Id<Node> parent) {
	check(node);
	if (parent == null_id_v) { return add_unchecked(m_tree.roots, std::move(node)); }
	if (auto* target = find(parent)) { return add_unchecked(target->m_children, std::move(node)); }
	throw Error{fmt::format("Scene {}: Invalid parent Node Id: {}", m_name, parent)};
}

std::vector<Texture> Scene::replace(std::vector<Texture>&& textures) { return std::exchange(m_storage.resources.textures.m_array, std::move(textures)); }

std::vector<StaticMesh> Scene::replace(std::vector<StaticMesh>&& static_meshes) {
	return std::exchange(m_storage.resources.static_meshes.m_array, std::move(static_meshes));
}

bool Scene::load(Id<Tree> id) {
	if (id >= tree_count()) { return false; }
	return load_tree(id);
}

Ptr<Node> Scene::find(Id<Node> id) { return const_cast<Node*>(std::as_const(*this).find_node(m_tree.roots, id)); }
Ptr<Node const> Scene::find(Id<Node> id) const { return find_node(m_tree.roots, id); }

Node& Scene::camera() { return const_cast<Node&>(std::as_const(*this).camera()); }

Node const& Scene::camera() const {
	auto* ret = find(m_tree.camera);
	assert(ret);
	return *ret;
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

Node Scene::make_camera_node(Id<Camera> id) const {
	assert(id < m_storage.resources.cameras.size());
	auto node = Node{};
	node.name = "camera";
	node.transform.set_position({0.0f, 0.0f, 5.0f});
	node.attach<Id<Camera>>(id);
	return node;
}

void Scene::add_default_camera() {
	m_tree.cameras.push_back(add_unchecked(m_tree.roots, make_camera_node(add(Camera{.name = "default"}))));
	m_tree.camera = m_tree.cameras.front();
}

bool Scene::load_tree(Id<Tree> id) {
	assert(id < m_storage.data.trees.size());
	m_tree = TreeBuilder{*this, m_storage.data.nodes}(m_storage.data.trees[id], id);
	return true;
}

Id<Mesh> Scene::add_unchecked(Mesh mesh) {
	auto const id = m_storage.resources.meshes.size();
	m_storage.resources.meshes.m_array.push_back(std::move(mesh));
	return id;
}

Id<Node> Scene::add_unchecked(std::vector<Node>& out, Node&& node) {
	auto const id = Id<Node>{++s_next_node};
	node.m_id = id;
	out.push_back(std::move(node));
	return id;
}

Node const* Scene::find_node(std::span<Node const> nodes, Id<Node> id) {
	for (auto const& node : nodes) {
		if (node.m_id == id) { return &node; }
		if (node.m_children.empty()) { continue; }
		if (auto const* ret = find_node(node.m_children, id)) { return ret; }
	}
	return nullptr;
}

void Scene::check(Mesh const& mesh) const noexcept(false) {
	for (auto const& primitive : mesh.primitives) {
		if (primitive.static_mesh >= m_storage.resources.static_meshes.size()) {
			throw Error{fmt::format("Scene {}: Invalid Static Mesh Id: {}", m_name, primitive.static_mesh)};
		}
		if (primitive.material && primitive.material->value() >= m_storage.resources.materials.size()) {
			throw Error{fmt::format("Scene {}: Invalid Material Id: {}", m_name, *primitive.material)};
		}
	}
}

void Scene::check(Node const& node) const noexcept(false) {
	if (auto const* mesh_id = node.find<Id<Mesh>>(); mesh_id && *mesh_id >= m_storage.resources.meshes.size()) {
		throw Error{fmt::format("Scene {}: Invalid mesh [{}] in node", m_name, *mesh_id)};
	}
	if (auto const* camera_id = node.find<Id<Camera>>(); camera_id && *camera_id >= m_storage.resources.cameras.size()) {
		throw Error{fmt::format("Scene {}: Invalid camera [{}] in node", m_name, *camera_id)};
	}
	for (auto const& child : node.m_children) { check(child); }
}
} // namespace facade
