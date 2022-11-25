#include <facade/scene/gltf_loader.hpp>
#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/thread_pool.hpp>
#include <facade/util/zip_ranges.hpp>
#include <facade/vk/geometry.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <gltf2cpp/gltf2cpp.hpp>

namespace facade {
namespace {
Camera to_camera(gltf2cpp::Camera cam) {
	auto ret = Camera{};
	ret.name = std::move(cam.name);
	auto visitor = Visitor{
		[&ret](gltf2cpp::Camera::Orthographic const& o) {
			auto orthographic = Camera::Orthographic{};
			orthographic.view_plane = {.near = o.znear, .far = o.zfar};
			ret.type = orthographic;
		},
		[&ret](gltf2cpp::Camera::Perspective const& p) {
			auto perspective = Camera::Perspective{};
			perspective.view_plane = {.near = p.znear, .far = p.zfar.value_or(10000.0f)};
			perspective.field_of_view = p.yfov;
			ret.type = perspective;
		},
	};
	std::visit(visitor, cam.payload);
	return ret;
}

constexpr vk::SamplerAddressMode to_address_mode(gltf2cpp::Wrap const wrap) {
	switch (wrap) {
	case gltf2cpp::Wrap::eClampEdge: return vk::SamplerAddressMode::eClampToEdge;
	case gltf2cpp::Wrap::eMirrorRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
	default:
	case gltf2cpp::Wrap::eRepeat: return vk::SamplerAddressMode::eRepeat;
	}
}

constexpr vk::Filter to_filter(gltf2cpp::Filter const filter) {
	switch (filter) {
	default:
	case gltf2cpp::Filter::eLinear: return vk::Filter::eLinear;
	case gltf2cpp::Filter::eNearest: return vk::Filter::eNearest;
	}
}

constexpr Material::AlphaMode to_alpha_mode(gltf2cpp::AlphaMode const mode) {
	switch (mode) {
	default:
	case gltf2cpp::AlphaMode::eOpaque: return Material::AlphaMode::eOpaque;
	case gltf2cpp::AlphaMode::eBlend: return Material::AlphaMode::eBlend;
	case gltf2cpp::AlphaMode::eMask: return Material::AlphaMode::eMask;
	}
}

Sampler::CreateInfo to_sampler_info(gltf2cpp::Sampler const& sampler) {
	auto ret = Sampler::CreateInfo{};
	ret.mode_s = to_address_mode(sampler.wrap_s);
	ret.mode_t = to_address_mode(sampler.wrap_t);
	if (sampler.min_filter) { ret.min = to_filter(*sampler.min_filter); }
	if (sampler.mag_filter) { ret.mag = to_filter(*sampler.mag_filter); }
	return ret;
}

Material to_material(gltf2cpp::Material const& material) {
	auto ret = std::make_unique<LitMaterial>();
	ret->albedo = {material.pbr.base_color_factor[0], material.pbr.base_color_factor[1], material.pbr.base_color_factor[2]};
	ret->metallic = material.pbr.metallic_factor;
	ret->roughness = material.pbr.roughness_factor;
	ret->alpha_mode = to_alpha_mode(material.alpha_mode);
	ret->alpha_cutoff = material.alpha_cutoff;
	if (material.pbr.base_color_texture) { ret->base_colour = material.pbr.base_color_texture->texture; }
	if (material.pbr.metallic_roughness_texture) { ret->roughness_metallic = material.pbr.metallic_roughness_texture->texture; }
	if (material.emissive_texture) { ret->emissive = material.emissive_texture->texture; }
	ret->emissive_factor = {material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]};
	return {std::move(ret)};
}

template <glm::length_t Dim>
constexpr glm::vec<Dim, float> from_gltf(gltf2cpp::Vec<Dim> const& in) {
	auto ret = glm::vec<Dim, float>{};
	std::memcpy(&ret, &in, sizeof(in));
	return ret;
}

Geometry to_geometry(gltf2cpp::Mesh::Primitive&& primitive) {
	auto ret = Geometry{};
	for (std::size_t i = 0; i < primitive.geometry.positions.size(); ++i) {
		auto vertex = Vertex{.position = from_gltf<3>(primitive.geometry.positions[i])};
		if (!primitive.geometry.colors.empty()) { vertex.rgb = from_gltf<3>(primitive.geometry.colors[0][i]); }
		if (!primitive.geometry.normals.empty()) { vertex.normal = from_gltf<3>(primitive.geometry.normals[i]); }
		if (!primitive.geometry.tex_coords.empty()) { vertex.uv = from_gltf<2>(primitive.geometry.tex_coords[0][i]); }
		ret.vertices.push_back(vertex);
	}
	ret.indices = std::move(primitive.geometry.indices);
	return ret;
}

struct MeshData {
	struct Primitive {
		Geometry geometry{};
		std::optional<std::size_t> material{};
	};

	std::vector<Primitive> primitives{};
};

struct MeshLayout {
	struct Data {
		std::string name{};
		std::vector<Mesh::Primitive> primitives{};
	};

	std::vector<Geometry> geometries{};
	std::vector<Data> data{};
};

MeshLayout to_mesh_layout(gltf2cpp::Root& out_root) {
	auto ret = MeshLayout{};
	for (auto& mesh : out_root.meshes) {
		auto data = MeshLayout::Data{};
		data.name = std::move(mesh.name);
		for (auto& primitive : mesh.primitives) {
			auto mp = Mesh::Primitive{.static_mesh = ret.geometries.size()};
			ret.geometries.push_back(to_geometry(std::move(primitive)));
			mp.material = primitive.material;
			data.primitives.push_back(std::move(mp));
		}
		ret.data.push_back(std::move(data));
	}
	return ret;
}

Transform to_transform(gltf2cpp::Transform const& transform) {
	auto ret = Transform{};
	auto visitor = Visitor{
		[&ret](gltf2cpp::Trs const& trs) {
			ret.set_position({trs.translation[0], trs.translation[1], trs.translation[2]});
			ret.set_orientation({trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3]});
			ret.set_scale({trs.scale[0], trs.scale[1], trs.scale[2]});
		},
		[&ret](gltf2cpp::Mat4x4 const& mat) {
			auto m = glm::mat4x4{};
			m[0] = {mat[0][0], mat[0][1], mat[0][2], mat[0][3]};
			m[1] = {mat[1][0], mat[1][1], mat[1][2], mat[1][3]};
			m[2] = {mat[2][0], mat[2][1], mat[2][2], mat[2][3]};
			m[3] = {mat[3][0], mat[3][1], mat[3][2], mat[3][3]};
			glm::vec3 scale, pos, skew;
			glm::vec4 persp;
			glm::quat orn;
			glm::decompose(m, scale, orn, pos, skew, persp);
			ret.set_position(pos);
			ret.set_orientation(orn);
			ret.set_scale(scale);
		},
	};
	std::visit(visitor, transform);
	return ret;
}

NodeData to_node_data(gltf2cpp::Node&& node) {
	return NodeData{
		.name = std::move(node.name),
		.transform = to_transform(node.transform),
		.children = std::move(node.children),
		.camera = node.camera,
		.mesh = node.mesh,
	};
}

std::vector<NodeData> to_node_data(std::span<gltf2cpp::Node> nodes) {
	auto ret = std::vector<NodeData>{};
	ret.reserve(nodes.size());
	for (auto& in : nodes) {
		auto& node = ret.emplace_back(to_node_data(std::move(in)));
		node.index = ret.size() - 1;
	}
	return ret;
}

std::vector<Node> to_nodes(std::span<gltf2cpp::Node> nodes) {
	auto ret = std::vector<Node>{};
	ret.reserve(nodes.size());
	for (auto& in : nodes) {
		auto node = Node{.transform = to_transform(in.transform), .self = ret.size(), .name = std::move(in.name)};
		node.children.reserve(in.children.size());
		for (auto const& child : in.children) { node.children.push_back(child); }
		if (in.camera) { node.attach<Camera>(*in.camera); }
		if (in.mesh) { node.attach<Mesh>(*in.mesh); }
		ret.push_back(std::move(node));
	}
	return ret;
}

template <typename T>
Interpolator<T> make_interpolator(std::span<float const> times, std::span<T const> values) {
	assert(times.size() == values.size());
	auto ret = Interpolator<T>{};
	for (auto [t, v] : zip_ranges(times, values)) { ret.keyframes.push_back({v, t}); }
	return ret;
}

Animation to_animation(gltf2cpp::Animation const& animation, std::span<gltf2cpp::Accessor const> accessors) {
	using Path = gltf2cpp::Animation::Path;
	auto ret = Animation{};
	for (auto const& channel : animation.channels) {
		auto const& sampler = animation.samplers[channel.sampler];
		auto const& input = accessors[sampler.input];
		assert(input.type == gltf2cpp::Accessor::Type::eScalar && input.component_type == gltf2cpp::ComponentType::eFloat);
		auto times = std::get<gltf2cpp::Accessor::Float>(input.data).span();
		auto const& output = accessors[sampler.output];
		assert(output.component_type == gltf2cpp::ComponentType::eFloat);
		auto const values = std::get<gltf2cpp::Accessor::Float>(output.data).span();
		switch (channel.target.path) {
		case Path::eTranslation:
		case Path::eScale: {
			assert(output.type == gltf2cpp::Accessor::Type::eVec3);
			auto vec = std::vector<glm::vec3>{};
			vec.resize(values.size() / 3);
			std::memcpy(vec.data(), values.data(), values.size_bytes());
			if (channel.target.path == Path::eScale) {
				ret.animator.scale = make_interpolator<glm::vec3>(times, vec);
			} else {
				ret.animator.translation = make_interpolator<glm::vec3>(times, vec);
			}
			break;
		}
		case Path::eRotation: {
			assert(output.type == gltf2cpp::Accessor::Type::eVec4);
			auto vec = std::vector<glm::quat>{};
			vec.resize(values.size() / 4);
			std::memcpy(vec.data(), values.data(), values.size_bytes());
			ret.animator.rotation = make_interpolator<glm::quat>(times, vec);
			break;
		}
		case Path::eWeights: {
			// TODO not implemented
			break;
		}
		}
	}
	return ret;
}

template <typename F>
auto make_load_future(ThreadPool* pool, std::atomic<std::size_t>& out_done, F func) -> LoadFuture<std::invoke_result_t<F>> {
	if (pool) { return {*pool, out_done, std::move(func)}; }
	return {out_done, std::move(func)};
}

template <typename T>
std::vector<T> from_maybe_futures(std::vector<MaybeFuture<T>>&& futures) {
	auto ret = std::vector<T>{};
	ret.reserve(futures.size());
	for (auto& future : futures) { ret.push_back(future.get()); }
	return ret;
}
} // namespace

bool Scene::GltfLoader::operator()(dj::Json const& json, DataProvider const& provider, ThreadPool* thread_pool) noexcept(false) {
	auto const parser = gltf2cpp::Parser{json};
	auto const meta = parser.metadata();
	m_status.done = 0;
	m_status.total = 1 + meta.images + meta.textures + meta.primitives + 1;
	m_status.stage = LoadStage::eParsingJson;

	auto get_bytes = [&provider](std::string_view uri) {
		auto ret = provider.load(uri);
		return gltf2cpp::ByteArray{std::move(ret.bytes), ret.size};
	};

	auto root = parser.parse(get_bytes);
	if (!root || root.meshes.empty() || root.scenes.empty()) { return false; }
	if (root.start_scene && *root.start_scene >= root.scenes.size()) { throw Error{fmt::format("Invalid start scene: {}", *root.start_scene)}; }

	++m_status.done;
	m_status.stage = LoadStage::eUploadingResources;
	m_scene.m_storage = {};

	auto images = std::vector<MaybeFuture<Image>>{};
	images.reserve(root.images.size());
	for (auto& image : root.images) {
		images.push_back(make_load_future(thread_pool, m_status.done, [i = std::move(image)] { return Image{i.bytes.span(), std::move(i.name)}; }));
	}

	for (auto const& sampler : root.samplers) { m_scene.add(to_sampler_info(sampler)); }
	auto get_sampler = [this](std::optional<std::size_t> sampler_id) {
		if (!sampler_id || sampler_id >= m_scene.m_storage.resources.samplers.size()) { return m_scene.default_sampler(); }
		return m_scene.m_storage.resources.samplers[*sampler_id].sampler();
	};

	auto textures = std::vector<MaybeFuture<Texture>>{};
	textures.reserve(root.textures.size());
	for (auto& texture : root.textures) {
		textures.push_back(make_load_future(thread_pool, m_status.done, [texture = std::move(texture), &images, &get_sampler, this] {
			bool const mip_mapped = !texture.linear;
			auto const colour_space = texture.linear ? ColourSpace::eLinear : ColourSpace::eSrgb;
			auto const tci = Texture::CreateInfo{.name = std::move(texture.name), .mip_mapped = mip_mapped, .colour_space = colour_space};
			return Texture{m_scene.m_gfx, get_sampler(texture.sampler), images[texture.source].get(), tci};
		}));
	}

	auto static_meshes = std::vector<MaybeFuture<StaticMesh>>{};
	auto mesh_layout = to_mesh_layout(root);
	static_meshes.reserve(mesh_layout.geometries.size());
	for (auto& geometry : mesh_layout.geometries) {
		static_meshes.push_back(make_load_future(thread_pool, m_status.done, [g = std::move(geometry), this] { return StaticMesh{m_scene.m_gfx, g}; }));
	}

	for (auto const& animation : root.animations) { m_scene.m_storage.resources.animations.m_array.push_back(to_animation(animation, root.accessors)); }

	m_scene.m_storage.resources.nodes.m_array = to_nodes(root.nodes);

	m_scene.replace(from_maybe_futures(std::move(textures)));
	m_scene.replace(from_maybe_futures(std::move(static_meshes)));

	m_status.stage = LoadStage::eBuildingScenes;
	if (root.cameras.empty()) {
		m_scene.add(Camera{.name = "default"});
	} else {
		for (auto gltf_camera : root.cameras) { m_scene.add(to_camera(std::move(gltf_camera))); }
	}
	for (auto const& material : root.materials) { m_scene.add(to_material(material)); }
	for (auto& data : mesh_layout.data) { m_scene.add(Mesh{.name = std::move(data.name), .primitives = std::move(data.primitives)}); }

	m_scene.m_storage.data.nodes = to_node_data(root.nodes);
	for (auto& scene : root.scenes) {
		auto& roots = m_scene.m_storage.data.trees.emplace_back();
		for (auto id : scene.root_nodes) { roots.push_back(id); }
	}

	auto const ret = m_scene.load(root.start_scene.value_or(0));
	m_status.reset();
	return ret;
}
} // namespace facade
