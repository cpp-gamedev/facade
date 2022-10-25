#include <detail/gltf.hpp>
#include <djson/json.hpp>
#include <facade/render/renderer.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/vk/texture.hpp>
#include <map>

namespace facade {
namespace {
Camera to_camera(gltf::Camera cam) {
	auto ret = Camera{};
	ret.name = std::move(cam.name);
	switch (cam.type) {
	case gltf::Camera::Type::eOrthographic: {
		auto orthographic = Camera::Orthographic{};
		orthographic.view_plane = {.near = cam.orthographic.znear, .far = cam.orthographic.zfar};
		ret.type = orthographic;
		break;
	}
	default: {
		auto perspective = Camera::Perspective{};
		perspective.view_plane = {.near = cam.perspective.znear, .far = cam.perspective.zfar.value_or(10000.0f)};
		perspective.field_of_view = cam.perspective.yfov;
		break;
	}
	}
	return ret;
}

constexpr vk::SamplerAddressMode to_address_mode(gltf::Wrap const wrap) {
	switch (wrap) {
	case gltf::Wrap::eClampEdge: return vk::SamplerAddressMode::eClampToEdge;
	case gltf::Wrap::eMirrorRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
	default:
	case gltf::Wrap::eRepeat: return vk::SamplerAddressMode::eRepeat;
	}
}

constexpr vk::Filter to_filter(gltf::Filter const filter) {
	switch (filter) {
	default:
	case gltf::Filter::eLinear: return vk::Filter::eLinear;
	case gltf::Filter::eNearest: return vk::Filter::eNearest;
	}
}

Sampler to_sampler(Gfx const& gfx, gltf::Sampler const& sampler) {
	auto info = Sampler::CreateInfo{};
	info.mode_s = to_address_mode(sampler.wrap_s);
	info.mode_t = to_address_mode(sampler.wrap_t);
	if (sampler.min_filter) { info.min = to_filter(*sampler.min_filter); }
	if (sampler.mag_filter) { info.mag = to_filter(*sampler.mag_filter); }
	return Sampler{gfx, info};
}

std::unique_ptr<Material> to_material(gltf::Material const& material) {
	auto ret = std::make_unique<LitMaterial>();
	ret->albedo = material.pbr.base_colour_factor;
	ret->metallic = material.pbr.metallic_factor;
	ret->roughness = material.pbr.roughness_factor;
	if (material.pbr.base_colour_texture) { ret->base_colour = material.pbr.base_colour_texture->texture; }
	if (material.pbr.metallic_roughness_texture) { ret->roughness_metallic = material.pbr.metallic_roughness_texture->texture; }
	return ret;
}

Mesh to_mesh(gltf::Mesh const& mesh) {
	auto ret = Mesh{};
	for (auto const& primitive : mesh.primitives) {
		ret.primitives.push_back(Mesh::Primitive{.static_mesh = primitive.geometry, .material = primitive.material});
	}
	return ret;
}

Texture to_texture(Gfx const& gfx, vk::Sampler sampler, Image const& image, gltf::Texture const& texture) {
	return Texture{gfx, sampler, image.view(), Texture::CreateInfo{.mip_mapped = true, .colour_space = texture.colour_space}};
}

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

	std::optional<Id<Node>> camera{};

	void add(NodeData const& child, std::vector<Node>& out_children) {
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
			add(child, node.m_children);
		}
		out_scene.check(node);
		auto const node_id = out_scene.add_unchecked(out_children, std::move(node));
		if (set_cam) { camera = node_id; }
	}

	Tree operator()(Tree::Data const& tree, Id<Scene> id) {
		auto ret = Tree{.id = id};
		for (auto const index : tree.roots) {
			assert(index < in_gnodes.size());
			add(in_gnodes[index], ret.roots);
		}
		if (camera) {
			ret.camera = *camera;
		} else {
			// add node with default camera
			assert(!out_scene.m_storage.cameras.empty());
			auto node = Node{};
			// TODO
			node.transform.set_position({0.0f, 0.0f, 5.0f});
			node.attach(Id<Camera>{0});
			ret.camera = out_scene.add_unchecked(ret.roots, std::move(node));
		}
		return ret;
	}
};

bool Scene::load_gltf(dj::Json const& root, DataProvider const& provider) noexcept(false) {
	auto asset = gltf::Asset::parse(root, provider);
	if (asset.geometries.empty() || asset.scenes.empty()) { return false; }
	if (asset.start_scene >= asset.scenes.size()) { throw Error{fmt::format("Invalid start scene: {}", asset.start_scene)}; }

	m_storage = {};
	if (asset.cameras.empty()) {
		add(Camera{.name = "default"});
	} else {
		for (auto gltf_camera : asset.cameras) { add(to_camera(std::move(gltf_camera))); }
	}

	m_storage.images = std::move(asset.images);
	for (auto const& sampler : asset.samplers) { add(to_sampler(m_gfx, sampler)); }
	for (auto const& material : asset.materials) { add(to_material(material)); }
	for (auto const& geometry : asset.geometries) { add(StaticMesh{m_gfx, geometry}); }
	for (auto const& mesh : asset.meshes) { add(to_mesh(mesh)); }

	auto get_sampler = [this](std::optional<std::size_t> sampler_id) {
		if (!sampler_id || sampler_id >= m_storage.samplers.size()) { return default_sampler(); }
		return m_storage.samplers[*sampler_id].sampler();
	};
	for (auto const& texture : asset.textures) {
		m_storage.textures.push_back(to_texture(m_gfx, get_sampler(texture.sampler), m_storage.images.at(texture.source), texture));
	}

	m_storage.data.nodes = std::move(asset.nodes);
	for (auto& scene : asset.scenes) { m_storage.data.trees.push_back(Tree::Data{.roots = std::move(scene.root_nodes)}); }

	return load(asset.start_scene);
}

Scene::Scene(Gfx const& gfx)
	: m_gfx(gfx), m_sampler(gfx), m_view_proj(gfx, Buffer::Type::eUniform), m_dir_lights(gfx, Buffer::Type::eStorage),
	  m_white(gfx, m_sampler.sampler(), Img1x1::make({0xff, 0xff, 0xff, 0xff}).view(), Texture::CreateInfo{.mip_mapped = false}) {}

Id<Camera> Scene::add(Camera camera) {
	auto const id = m_storage.cameras.size();
	m_storage.cameras.push_back(std::move(camera));
	return id;
}

Id<Sampler> Scene::add(Sampler sampler) {
	auto const id = m_storage.samplers.size();
	m_storage.samplers.push_back(std::move(sampler));
	return id;
}

Id<Material> Scene::add(std::unique_ptr<Material> material) {
	auto const id = m_storage.materials.size();
	m_storage.materials.push_back(std::move(material));
	return id;
}

Id<StaticMesh> Scene::add(StaticMesh mesh) {
	auto const id = m_storage.static_meshes.size();
	m_storage.static_meshes.push_back(std::move(mesh));
	return id;
}

Id<Image> Scene::add(Image image) {
	auto const id = m_storage.images.size();
	m_storage.images.push_back(std::move(image));
	return id;
}

Id<Mesh> Scene::add(Mesh mesh) {
	check(mesh);
	return add_unchecked(std::move(mesh));
}

Id<Node> Scene::add(Node node, Id<Node> parent) {
	check(node);
	if (parent == id_v) { return add_unchecked(m_tree.roots, std::move(node)); }
	if (auto* target = find_node(parent)) { return add_unchecked(target->m_children, std::move(node)); }
	throw Error{fmt::format("Scene {}: Invalid parent Node Id: {}", m_name, parent)};
}

bool Scene::load(Id<Scene> id) {
	if (id >= scene_count()) { return false; }
	return load_tree(id);
}

Node* Scene::find_node(Id<Node> id) { return const_cast<Node*>(std::as_const(*this).find_node(m_tree.roots, id)); }
Node const* Scene::find_node(Id<Node> id) const { return find_node(m_tree.roots, id); }

Material* Scene::find_material(Id<Material> id) const { return id >= m_storage.materials.size() ? nullptr : m_storage.materials[id].get(); }

Mesh const* Scene::find_mesh(Id<Mesh> id) const { return id >= m_storage.meshes.size() ? nullptr : &m_storage.meshes[id]; }

bool Scene::select_camera(Id<Camera> id) {
	if (id >= camera_count()) { return false; }
	*camera().find<Id<Camera>>() = id;
	return true;
}

Node& Scene::camera() {
	auto* ret = find_node(m_tree.camera);
	assert(ret);
	return *ret;
}

Texture Scene::make_texture(Image::View image) const { return Texture{m_gfx, default_sampler(), image}; }

void Scene::write_view(Pipeline& out_pipeline) const {
	auto& set0 = out_pipeline.next_set(0);
	set0.update(0, m_view_proj.descriptor_buffer());
	set0.update(1, m_dir_lights.descriptor_buffer());
	out_pipeline.bind(set0);
}

void Scene::render(Renderer& renderer, vk::CommandBuffer cb) {
	write_view(renderer.framebuffer_extent());
	for (auto const& node : m_tree.roots) { render(renderer, cb, node); }
}

bool Scene::load_tree(Id<Scene> id) {
	assert(id < m_storage.data.trees.size());
	m_tree = TreeBuilder{*this, m_storage.data.nodes}(m_storage.data.trees[id], id);
	return true;
}

Id<Mesh> Scene::add_unchecked(Mesh mesh) {
	auto const id = m_storage.meshes.size();
	m_storage.meshes.push_back(std::move(mesh));
	return id;
}

Id<Node> Scene::add_unchecked(std::vector<Node>& out, Node&& node) {
	m_storage.next_node = {m_storage.next_node + 1};
	node.m_id = m_storage.next_node;
	out.push_back(std::move(node));
	return m_storage.next_node;
}

Node const* Scene::find_node(std::span<Node const> nodes, Id<Node> id) {
	for (auto const& node : nodes) {
		if (node.m_id == id) { return &node; }
		if (node.m_children.empty()) { continue; }
		if (auto const* ret = find_node(node.m_children, id)) { return ret; }
	}
	return nullptr;
}

void Scene::write_view(glm::vec2 const extent) {
	auto const& cam_node = camera();
	auto const* cam_id = cam_node.find<Id<Camera>>();
	assert(cam_id);
	auto const& cam = m_storage.cameras[*cam_id];
	struct {
		glm::mat4x4 mat_v;
		glm::mat4x4 mat_p;
		glm::vec4 pos_v;
	} vp{
		.mat_v = cam.view(cam_node.transform),
		.mat_p = cam.projection(extent),
		.pos_v = {cam_node.transform.position(), 1.0f},
	};
	m_view_proj.write(&vp, sizeof(vp));
	m_dir_lights.write(dir_lights.data(), dir_lights.size() * sizeof(DirLight));
}

std::span<glm::mat4x4 const> Scene::make_instances(Node const& node, glm::mat4x4 const& parent) {
	m_storage.instances.clear();
	if (node.instances.empty()) {
		m_storage.instances.reserve(1);
		m_storage.instances.push_back(parent * node.transform.matrix());
	} else {
		m_storage.instances.reserve(node.instances.size());
		for (auto const& transform : node.instances) { m_storage.instances.push_back(parent * node.transform.matrix() * transform.matrix()); }
	}
	return m_storage.instances;
}

void Scene::render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4 const& parent) {
	if (auto const* mesh_id = node.find<Id<Mesh>>()) {
		static auto const s_default_material = LitMaterial{};
		auto const& mesh = m_storage.meshes.at(*mesh_id);
		for (auto const& primitive : mesh.primitives) {
			auto const& material = primitive.material ? *m_storage.materials.at(primitive.material->value()) : static_cast<Material const&>(s_default_material);
			auto pipeline = renderer.bind_pipeline(cb, {}, material.shader_id());

			write_view(pipeline);
			auto const store = TextureStore{m_storage.textures, m_white};
			material.write_sets(pipeline, store);

			auto const& static_mesh = m_storage.static_meshes.at(primitive.static_mesh);
			auto const instances = make_instances(node, parent);
			renderer.draw(pipeline, static_mesh, instances);
		}
	}

	for (auto const& child : node.m_children) { render(renderer, cb, child, parent * node.transform.matrix()); }
}

void Scene::check(Mesh const& mesh) const noexcept(false) {
	for (auto const& primitive : mesh.primitives) {
		if (primitive.static_mesh >= m_storage.static_meshes.size()) {
			throw Error{fmt::format("Scene {}: Invalid Static Mesh Id: {}", m_name, primitive.static_mesh)};
		}
		if (primitive.material && primitive.material->value() >= m_storage.materials.size()) {
			throw Error{fmt::format("Scene {}: Invalid Material Id: {}", m_name, *primitive.material)};
		}
	}
}

void Scene::check(Node const& node) const noexcept(false) {
	if (auto const* mesh_id = node.find<Id<Mesh>>(); mesh_id && *mesh_id >= m_storage.meshes.size()) {
		throw Error{fmt::format("Scene {}: Invalid mesh [{}] in node", m_name, *mesh_id)};
	}
	if (auto const* camera_id = node.find<Id<Camera>>(); camera_id && *camera_id >= m_storage.cameras.size()) {
		throw Error{fmt::format("Scene {}: Invalid camera [{}] in node", m_name, *camera_id)};
	}
	for (auto const& child : node.m_children) { check(child); }
}
} // namespace facade