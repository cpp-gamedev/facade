#include <detail/gltf.hpp>
#include <facade/scene/gltf_loader.hpp>
#include <facade/util/error.hpp>
#include <facade/util/thread_pool.hpp>

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

Sampler::CreateInfo to_sampler_info(gltf::Sampler const& sampler) {
	auto ret = Sampler::CreateInfo{};
	ret.mode_s = to_address_mode(sampler.wrap_s);
	ret.mode_t = to_address_mode(sampler.wrap_t);
	if (sampler.min_filter) { ret.min = to_filter(*sampler.min_filter); }
	if (sampler.mag_filter) { ret.mag = to_filter(*sampler.mag_filter); }
	return ret;
}

std::unique_ptr<Material> to_material(gltf::Material const& material) {
	auto ret = std::make_unique<LitMaterial>();
	ret->albedo = material.pbr.base_colour_factor;
	ret->metallic = material.pbr.metallic_factor;
	ret->roughness = material.pbr.roughness_factor;
	ret->alpha_mode = material.alpha_mode;
	ret->alpha_cutoff = material.alpha_cutoff;
	if (material.pbr.base_colour_texture) { ret->base_colour = material.pbr.base_colour_texture->texture; }
	if (material.pbr.metallic_roughness_texture) { ret->roughness_metallic = material.pbr.metallic_roughness_texture->texture; }
	return ret;
}

Mesh to_mesh(gltf::Mesh mesh) {
	auto ret = Mesh{.name = std::move(mesh.name)};
	for (auto const& primitive : mesh.primitives) {
		ret.primitives.push_back(Mesh::Primitive{.static_mesh = primitive.geometry, .material = primitive.material});
	}
	return ret;
}

template <typename T>
struct MaybeFuture {
	std::future<T> future{};
	std::optional<T> t{};

	template <typename F>
		requires(std::same_as<std::invoke_result_t<F>, T>)
	MaybeFuture(ThreadPool& pool, std::atomic<std::size_t>& done, F func)
		: future(pool.enqueue([&done, f = std::move(func)] {
			  auto ret = f();
			  ++done;
			  return ret;
		  })) {}

	template <typename F>
		requires(std::same_as<std::invoke_result_t<F>, T>)
	MaybeFuture(std::atomic<std::size_t>& done, F func) : t(func()) {
		++done;
	}

	T get() {
		assert(future.valid() || t.has_value());
		if (future.valid()) { return future.get(); }
		return std::move(*t);
	}
};

template <typename F>
static auto make_maybe_future(ThreadPool* pool, std::atomic<std::size_t>& out_done, F func) -> MaybeFuture<std::invoke_result_t<F>> {
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

bool Scene::GltfLoader::operator()(dj::Json const& root, DataProvider const& provider, ThreadPool* thread_pool) noexcept(false) {
	auto const meta = gltf::Asset::peek(root);
	m_status.done = 0;
	m_status.total = 1 + meta.images + meta.textures + meta.primitives + 1;
	m_status.stage = LoadStage::eParsingJson;

	auto asset = gltf::Asset::parse(root, provider);
	if (asset.geometries.empty() || asset.scenes.empty()) { return false; }
	if (asset.start_scene >= asset.scenes.size()) { throw Error{fmt::format("Invalid start scene: {}", asset.start_scene)}; }

	++m_status.done;
	m_status.stage = LoadStage::eUploadingResources;
	m_scene.m_storage = {};

	auto images = std::vector<MaybeFuture<Image>>{};
	images.reserve(asset.images.size());
	for (auto& image : asset.images) {
		images.push_back(make_maybe_future(thread_pool, m_status.done, [i = std::move(image)] { return Image{i.bytes.span(), std::move(i.name)}; }));
	}

	for (auto const& sampler : asset.samplers) { m_scene.add(to_sampler_info(sampler)); }
	auto get_sampler = [this](std::optional<std::size_t> sampler_id) {
		if (!sampler_id || sampler_id >= m_scene.m_storage.samplers.size()) { return m_scene.default_sampler(); }
		return m_scene.m_storage.samplers[*sampler_id].sampler();
	};

	auto textures = std::vector<MaybeFuture<Texture>>{};
	textures.reserve(asset.textures.size());
	for (auto& texture : asset.textures) {
		textures.push_back(make_maybe_future(thread_pool, m_status.done, [texture = std::move(texture), &images, &get_sampler, this] {
			bool const mip_mapped = texture.colour_space == ColourSpace::eSrgb;
			auto const tci = Texture::CreateInfo{.name = std::move(texture.name), .mip_mapped = mip_mapped, .colour_space = texture.colour_space};
			return Texture{m_scene.m_gfx, get_sampler(texture.sampler), images[texture.source].get(), tci};
		}));
	}

	auto static_meshes = std::vector<MaybeFuture<StaticMesh>>{};
	static_meshes.reserve(asset.geometries.size());
	for (auto& geometry : asset.geometries) {
		static_meshes.push_back(make_maybe_future(thread_pool, m_status.done, [g = std::move(geometry), this] { return StaticMesh{m_scene.m_gfx, g}; }));
	}

	m_scene.m_storage.textures = from_maybe_futures(std::move(textures));
	m_scene.m_storage.static_meshes = from_maybe_futures(std::move(static_meshes));

	m_status.stage = LoadStage::eBuildingScenes;
	if (asset.cameras.empty()) {
		m_scene.add(Camera{.name = "default"});
	} else {
		for (auto gltf_camera : asset.cameras) { m_scene.add(to_camera(std::move(gltf_camera))); }
	}
	for (auto const& material : asset.materials) { m_scene.add(to_material(material)); }
	for (auto& mesh : asset.meshes) { m_scene.add(to_mesh(std::move(mesh))); }

	m_scene.m_storage.data.nodes = std::move(asset.nodes);
	for (auto& scene : asset.scenes) { m_scene.m_storage.data.trees.push_back(TreeImpl::Data{.roots = std::move(scene.root_nodes)}); }

	auto const ret = m_scene.load(asset.start_scene);
	m_status.reset();
	return ret;
}
} // namespace facade
