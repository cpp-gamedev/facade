#include <detail/gltf.hpp>
#include <facade/scene/loader.hpp>
#include <facade/util/error.hpp>
#include <future>

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
} // namespace

bool Scene::Loader::operator()(dj::Json const& root, DataProvider const& provider) noexcept(false) {
	auto const meta = gltf::Asset::peek(root);
	m_status.done = 0;
	m_status.total = 1 + meta.images + meta.textures + meta.primitives + 1;
	m_status.stage = LoadStage::eParsingJson;

	auto asset = gltf::Asset::parse(root, provider);
	if (asset.geometries.empty() || asset.scenes.empty()) { return false; }
	if (asset.start_scene >= asset.scenes.size()) { throw Error{fmt::format("Invalid start scene: {}", asset.start_scene)}; }

	++m_status.done;
	m_status.stage = LoadStage::eLoadingImages;
	auto images = std::vector<Image>{};
	auto image_futures = std::vector<std::future<Image>>{};
	images.reserve(asset.images.size());
	for (auto& image : asset.images) {
		// image_futures.push_back(std::async([&image] { return Image{image.bytes.span(), std::move(image.name)}; }));
		images.emplace_back(image.bytes.span(), std::move(image.name));
		++m_status.done;
	}

	m_scene.m_storage = {};
	m_status.stage = LoadStage::eUploadingTextures;
	auto get_sampler = [this](std::optional<std::size_t> sampler_id) {
		if (!sampler_id || sampler_id >= m_scene.m_storage.samplers.size()) { return m_scene.default_sampler(); }
		return m_scene.m_storage.samplers[*sampler_id].sampler();
	};
	auto texture_futures = std::vector<std::future<Texture>>{};
	for (auto& texture : asset.textures) {
		auto const tci = Texture::CreateInfo{
			.name = std::move(texture.name), .mip_mapped = texture.colour_space == ColourSpace::eSrgb, .colour_space = texture.colour_space};
		m_scene.m_storage.textures.emplace_back(m_scene.m_gfx, get_sampler(texture.sampler), images[texture.source], tci);
		// texture_futures.push_back(std::async([&] {
		// 	bool const mip_mapped = texture.colour_space == ColourSpace::eSrgb;
		// 	auto const tci = Texture::CreateInfo{.name = std::move(texture.name), .mip_mapped = mip_mapped, .colour_space = texture.colour_space};
		// 	return Texture{m_out.m_gfx, get_sampler(texture.sampler), image_futures[texture.source].get(), tci};
		// }));
		++m_status.done;
	}

	m_status.stage = LoadStage::eUploadingMeshes;
	for (auto const& geometry : asset.geometries) {
		m_scene.add(geometry);
		++m_status.done;
	}

	m_status.stage = LoadStage::eBuildingScenes;
	if (asset.cameras.empty()) {
		m_scene.add(Camera{.name = "default"});
	} else {
		for (auto gltf_camera : asset.cameras) { m_scene.add(to_camera(std::move(gltf_camera))); }
	}
	for (auto const& sampler : asset.samplers) { m_scene.add(to_sampler_info(sampler)); }
	// for (auto& future : texture_futures) { m_out.m_storage.textures.push_back(future.get()); }
	for (auto const& material : asset.materials) { m_scene.add(to_material(material)); }
	for (auto& mesh : asset.meshes) { m_scene.add(to_mesh(std::move(mesh))); }

	m_scene.m_storage.data.nodes = std::move(asset.nodes);
	for (auto& scene : asset.scenes) { m_scene.m_storage.data.trees.push_back(TreeImpl::Data{.roots = std::move(scene.root_nodes)}); }

	auto const ret = m_scene.load(asset.start_scene);
	m_status.reset();
	return ret;
}
} // namespace facade
