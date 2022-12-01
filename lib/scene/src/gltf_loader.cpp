#include <facade/scene/gltf_loader.hpp>
#include <facade/util/data_provider.hpp>
#include <facade/util/enumerate.hpp>
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

constexpr AlphaMode to_alpha_mode(gltf2cpp::AlphaMode const mode) {
	switch (mode) {
	default:
	case gltf2cpp::AlphaMode::eOpaque: return AlphaMode::eOpaque;
	case gltf2cpp::AlphaMode::eBlend: return AlphaMode::eBlend;
	case gltf2cpp::AlphaMode::eMask: return AlphaMode::eMask;
	}
}

constexpr Topology to_topology(gltf2cpp::PrimitiveMode mode) {
	switch (mode) {
	case gltf2cpp::PrimitiveMode::ePoints: return Topology::ePoints;
	case gltf2cpp::PrimitiveMode::eLines: return Topology::eLines;
	case gltf2cpp::PrimitiveMode::eLineStrip: return Topology::eLineStrip;
	case gltf2cpp::PrimitiveMode::eLineLoop: break;
	case gltf2cpp::PrimitiveMode::eTriangles: return Topology::eTriangles;
	case gltf2cpp::PrimitiveMode::eTriangleStrip: return Topology::eTriangleStrip;
	case gltf2cpp::PrimitiveMode::eTriangleFan: return Topology::eTriangleFan;
	}
	throw Error{"Unsupported primitive mode: " + std::to_string(static_cast<int>(mode))};
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
	auto ret = LitMaterial{};
	ret.albedo = {material.pbr.base_color_factor[0], material.pbr.base_color_factor[1], material.pbr.base_color_factor[2]};
	ret.metallic = material.pbr.metallic_factor;
	ret.roughness = material.pbr.roughness_factor;
	ret.alpha_mode = to_alpha_mode(material.alpha_mode);
	ret.alpha_cutoff = material.alpha_cutoff;
	if (material.pbr.base_color_texture) { ret.base_colour = material.pbr.base_color_texture->texture; }
	if (material.pbr.metallic_roughness_texture) { ret.roughness_metallic = material.pbr.metallic_roughness_texture->texture; }
	if (material.emissive_texture) { ret.emissive = material.emissive_texture->texture; }
	ret.emissive_factor = {material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]};
	return Material{std::move(ret)};
}

glm::mat4x4 from_gltf(gltf2cpp::Mat4x4 const& in) {
	auto ret = glm::mat4x4{};
	ret[0] = {in[0][0], in[0][1], in[0][2], in[0][3]};
	ret[1] = {in[1][0], in[1][1], in[1][2], in[1][3]};
	ret[2] = {in[2][0], in[2][1], in[2][2], in[2][3]};
	ret[3] = {in[3][0], in[3][1], in[3][2], in[3][3]};
	return ret;
}

template <glm::length_t Dim>
std::vector<glm::vec<Dim, float>> from_gltf(std::vector<gltf2cpp::Vec<Dim>> const in) {
	if (in.empty()) { return {}; }
	auto ret = std::vector<glm::vec<Dim, float>>(in.size());
	std::memcpy(ret.data(), in.data(), std::span{in}.size_bytes());
	return ret;
}

Geometry to_geometry(gltf2cpp::Mesh::Primitive&& primitive) {
	auto ret = Geometry{};
	ret.positions = from_gltf<3>(primitive.geometry.positions);
	if (!primitive.geometry.colors.empty()) { ret.rgbs = from_gltf<3>(primitive.geometry.colors[0]); }
	if (ret.rgbs.empty()) { ret.rgbs = std::vector<glm::vec3>(ret.positions.size(), glm::vec3{1.0f}); }
	ret.normals = from_gltf<3>(primitive.geometry.normals);
	if (ret.normals.empty()) { ret.normals = std::vector<glm::vec3>(ret.positions.size(), glm::vec3{0.0f, 0.0f, 1.0f}); }
	if (!primitive.geometry.tex_coords.empty()) { ret.uvs = from_gltf<2>(primitive.geometry.tex_coords[0]); }
	if (ret.uvs.empty()) { ret.uvs = std::vector<glm::vec2>(ret.positions.size()); }
	ret.indices = std::move(primitive.geometry.indices);
	return ret;
}

struct MeshLayout {
	struct Data {
		std::string name{};
		std::vector<Mesh::Primitive> primitives{};
	};

	struct Static {
		Geometry geometry{};
	};

	struct Skinned : Static {
		std::vector<glm::uvec4> joints{};
		std::vector<glm::vec4> weights{};
	};

	std::vector<Static> static_meshes{};
	std::vector<Skinned> skinned_meshes{};
	std::vector<Data> data{};
};

MeshLayout::Skinned to_skinned_mesh_layout(gltf2cpp::Mesh::Primitive&& primitive) {
	auto ret = MeshLayout::Skinned{};
	ret.joints.resize(primitive.geometry.joints[0].size());
	std::memcpy(ret.joints.data(), primitive.geometry.joints[0].data(), std::span{primitive.geometry.joints[0]}.size_bytes());
	ret.weights.resize(primitive.geometry.weights[0].size());
	std::memcpy(ret.weights.data(), primitive.geometry.weights[0].data(), std::span{primitive.geometry.weights[0]}.size_bytes());
	ret.geometry = to_geometry(std::move(primitive));
	return ret;
}

MeshLayout::Static to_static_mesh_layout(gltf2cpp::Mesh::Primitive&& primitive) {
	auto ret = MeshLayout::Static{};
	ret.geometry = to_geometry(std::move(primitive));
	return ret;
}

MeshLayout to_mesh_layout(gltf2cpp::Root& out_root) {
	auto ret = MeshLayout{};
	for (auto& mesh : out_root.meshes) {
		auto data = MeshLayout::Data{};
		data.name = std::move(mesh.name);
		for (auto& primitive : mesh.primitives) {
			auto const topology = to_topology(primitive.mode);
			if (!primitive.geometry.joints.empty()) {
				data.primitives.push_back(Mesh::Primitive{
					.skinned_mesh = ret.skinned_meshes.size(),
					.material = primitive.material,
					.topology = topology,
				});
				ret.skinned_meshes.push_back(to_skinned_mesh_layout(std::move(primitive)));
			} else {
				data.primitives.push_back(Mesh::Primitive{
					.static_mesh = ret.static_meshes.size(),
					.material = primitive.material,
					.topology = topology,
				});
				ret.static_meshes.push_back(to_static_mesh_layout(std::move(primitive)));
			}
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
			auto m = from_gltf(mat);
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
		.skin = node.skin,
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
		if (in.skin) { node.attach<Skin>(*in.skin); }
		for (float const weight : in.weights) {
			if (node.weights.weights.size() < MorphWeights::max_weights_v) { node.weights.weights.insert(weight); }
		}
		ret.push_back(std::move(node));
	}
	return ret;
}

template <typename T>
Interpolator<T> make_interpolator(std::span<float const> times, std::span<T const> values, gltf2cpp::Interpolation interpolation) {
	assert(times.size() == values.size());
	auto ret = Interpolator<T>{};
	for (auto [t, v] : zip_ranges(times, values)) { ret.keyframes.push_back({v, t}); }
	ret.interpolation = static_cast<Interpolation>(interpolation);
	return ret;
}

Animation to_animation(gltf2cpp::Animation const& animation, std::span<gltf2cpp::Accessor const> accessors) {
	using Path = gltf2cpp::Animation::Path;
	auto ret = Animation{};
	for (auto const& channel : animation.channels) {
		auto const& sampler = animation.samplers[channel.sampler];
		if (sampler.interpolation == gltf2cpp::Interpolation::eCubicSpline) { continue; } // facade constraint
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
				ret.animator.scale.target = channel.target.node;
				ret.animator.scale.interpolator = make_interpolator<glm::vec3>(times, vec, sampler.interpolation);
			} else {
				ret.animator.translation.target = channel.target.node;
				ret.animator.translation.interpolator = make_interpolator<glm::vec3>(times, vec, sampler.interpolation);
			}
			break;
		}
		case Path::eRotation: {
			assert(output.type == gltf2cpp::Accessor::Type::eVec4);
			auto vec = std::vector<glm::quat>{};
			vec.resize(values.size() / 4);
			std::memcpy(vec.data(), values.data(), values.size_bytes());
			ret.animator.rotation.target = channel.target.node;
			ret.animator.rotation.interpolator = make_interpolator<glm::quat>(times, vec, sampler.interpolation);
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

Skin to_skin(gltf2cpp::Skin&& skin, std::span<gltf2cpp::Accessor const> accessors) {
	auto ret = Skin{};
	if (skin.inverse_bind_matrices) {
		auto const ibm = accessors[*skin.inverse_bind_matrices].to_mat4();
		assert(ibm.size() >= skin.joints.size());
		ret.inverse_bind_matrices.reserve(ibm.size());
		for (auto const& mat : ibm) { ret.inverse_bind_matrices.push_back(from_gltf(mat)); }
	} else {
		ret.inverse_bind_matrices = std::vector<glm::mat4x4>(ret.joints.size(), glm::identity<glm::mat4x4>());
	}
	ret.joints.reserve(skin.joints.size());
	for (auto const& joint : skin.joints) { ret.joints.push_back(joint); }
	ret.name = std::move(skin.name);
	ret.skeleton = skin.skeleton;
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
	auto skinned_meshes = std::vector<MaybeFuture<SkinnedMesh>>{};
	auto mesh_layout = to_mesh_layout(root);
	static_meshes.reserve(mesh_layout.static_meshes.size());
	skinned_meshes.reserve(mesh_layout.skinned_meshes.size());
	for (auto& data : mesh_layout.data) {
		for (auto [primitive, index] : enumerate(data.primitives)) {
			auto name = fmt::format("{}_{}", data.name, index);
			if (primitive.skinned_mesh) {
				auto& mesh = mesh_layout.skinned_meshes[*primitive.skinned_mesh];
				skinned_meshes.push_back(make_load_future(thread_pool, m_status.done, [mesh = std::move(mesh), name = std::move(name), this] {
					return SkinnedMesh{m_scene.m_gfx, mesh.geometry, {mesh.joints, mesh.weights}, std::move(name)};
				}));
			} else {
				auto& p = mesh_layout.static_meshes[primitive.static_mesh];
				static_meshes.push_back(make_load_future(thread_pool, m_status.done, [p = std::move(p), n = std::move(name), this] {
					return StaticMesh{m_scene.m_gfx, p.geometry, std::move(n)};
				}));
			}
		}
	}

	for (auto const& animation : root.animations) { m_scene.m_storage.resources.animations.m_array.push_back(to_animation(animation, root.accessors)); }
	for (auto& skin : root.skins) { m_scene.m_storage.resources.skins.m_array.push_back(to_skin(std::move(skin), root.accessors)); }

	m_scene.m_storage.resources.nodes.m_array = to_nodes(root.nodes);

	m_scene.replace(from_maybe_futures(std::move(textures)));
	m_scene.replace(from_maybe_futures(std::move(static_meshes)));
	m_scene.replace(from_maybe_futures(std::move(skinned_meshes)));

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
