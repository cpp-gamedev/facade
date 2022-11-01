#include <facade/engine/scene_renderer.hpp>

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

struct DirLightSSBO {
	alignas(16) glm::vec3 direction{front_v};
	alignas(16) glm::vec3 ambient{0.04f};
	alignas(16) glm::vec3 diffuse{1.0f};

	static DirLightSSBO make(DirLight const& light) {
		return {
			.direction = light.direction.value(),
			.diffuse = light.rgb.to_vec4(),
		};
	}
};
} // namespace

SceneRenderer::SceneRenderer(Gfx const& gfx)
	: m_gfx(gfx), m_sampler(gfx), m_view_proj(gfx, Buffer::Type::eUniform), m_dir_lights(gfx, Buffer::Type::eStorage),
	  m_white(gfx, m_sampler.sampler(), Img1x1::make({0xff, 0xff, 0xff, 0xff}).view(), Texture::CreateInfo{.mip_mapped = false}) {}

void SceneRenderer::render(Scene const& scene, Renderer& renderer, vk::CommandBuffer cb) {
	m_scene = &scene;
	write_view(renderer.framebuffer_extent());
	for (auto const& node : m_scene->m_tree.roots) { render(renderer, cb, node); }
}

void SceneRenderer::write_view(glm::vec2 const extent) {
	auto const& cam_node = m_scene->camera();
	auto const* cam_id = cam_node.find<Id<Camera>>();
	assert(cam_id);
	auto const& cam = m_scene->m_storage.cameras[*cam_id];
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
	auto dir_lights = FlexArray<DirLightSSBO, 4>{};
	for (auto const& light : m_scene->dir_lights.span()) { dir_lights.insert(DirLightSSBO::make(light)); }
	m_dir_lights.write(dir_lights.span().data(), dir_lights.span().size_bytes());
}

void SceneRenderer::update_view(Pipeline& out_pipeline) const {
	auto& set0 = out_pipeline.next_set(0);
	set0.update(0, m_view_proj.descriptor_buffer());
	set0.update(1, m_dir_lights.descriptor_buffer());
	out_pipeline.bind(set0);
}

std::span<glm::mat4x4 const> SceneRenderer::make_instances(Node const& node, glm::mat4x4 const& parent) {
	m_instance_mats.clear();
	if (node.instances.empty()) {
		m_instance_mats.reserve(1);
		m_instance_mats.push_back(parent * node.transform.matrix());
	} else {
		m_instance_mats.reserve(node.instances.size());
		for (auto const& transform : node.instances) { m_instance_mats.push_back(parent * node.transform.matrix() * transform.matrix()); }
	}
	return m_instance_mats;
}

void SceneRenderer::render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4 const& parent) {
	if (auto const* mesh_id = node.find<Id<Mesh>>()) {
		static auto const s_default_material = LitMaterial{};
		auto const& mesh = m_scene->m_storage.meshes.at(*mesh_id);
		for (auto const& primitive : mesh.primitives) {
			auto const& material =
				primitive.material ? *m_scene->m_storage.materials.at(primitive.material->value()) : static_cast<Material const&>(s_default_material);
			auto pipeline = renderer.bind_pipeline(cb, {}, material.shader_id());

			update_view(pipeline);
			auto const store = TextureStore{m_scene->m_storage.textures, m_white};
			material.write_sets(pipeline, store);

			auto const& static_mesh = m_scene->m_storage.static_meshes.at(primitive.static_mesh);
			auto const instances = make_instances(node, parent);
			renderer.draw(pipeline, static_mesh, instances);
		}
	}

	for (auto const& child : node.m_children) { render(renderer, cb, child, parent * node.transform.matrix()); }
}
} // namespace facade
