#include <facade/engine/scene_renderer.hpp>
#include <facade/vk/skybox.hpp>

namespace facade {
namespace {
using Bmp1x1 = FixedBitmap<1, 1>;

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

void SceneRenderer::Instances::rotate() {
	for (auto& buffer : buffers) { buffer.rotate(); }
	index = 0;
}

SceneRenderer::SceneRenderer(Gfx const& gfx)
	: m_gfx(gfx), m_sampler(gfx), m_view_proj(gfx, Buffer::Type::eUniform), m_dir_lights(gfx, Buffer::Type::eStorage),
	  m_white(gfx, m_sampler.sampler(), Bmp1x1{0xff_B, 0xff_B, 0xff_B, 0xff_B}.view(), Texture::CreateInfo{.mip_mapped = false}),
	  m_black(gfx, m_sampler.sampler(), Bmp1x1{0x0_B, 0x0_B, 0x0_B, 0xff_B}.view(), Texture::CreateInfo{.mip_mapped = false}) {}

void SceneRenderer::render(Scene const& scene, Ptr<Skybox const> skybox, Renderer& renderer, vk::CommandBuffer cb) {
	m_scene = &scene;
	m_info = {};
	write_view(renderer.framebuffer_extent());
	if (skybox) { render(renderer, cb, *skybox); }
	for (auto const& node : m_scene->roots()) { render(renderer, cb, m_scene->resources().nodes[node]); }
	m_instances.rotate();
}

void SceneRenderer::write_view(glm::vec2 const extent) {
	auto const& cam_node = m_scene->camera();
	auto const cam_id = cam_node.find<Camera>();
	assert(cam_id);
	auto const& cam = m_scene->resources().cameras[*cam_id];
	struct ViewSSBO {
		glm::mat4x4 mat_v;
		glm::mat4x4 mat_p;
		glm::vec4 vpos_exposure;
	} view{
		.mat_v = cam.view(cam_node.transform),
		.mat_p = cam.projection(extent),
		.vpos_exposure = {cam_node.transform.position(), cam.exposure},
	};
	m_view_proj.write<ViewSSBO>({&view, 1});
	auto dir_lights = FlexArray<DirLightSSBO, 4>{};
	for (auto const& light : m_scene->lights.dir_lights.span()) { dir_lights.insert(DirLightSSBO::make(light)); }
	m_dir_lights.write(dir_lights.span());
}

void SceneRenderer::update_view(Pipeline& out_pipeline) const {
	auto& set0 = out_pipeline.next_set(0);
	set0.update(0, m_view_proj.descriptor_buffer());
	set0.update(1, m_dir_lights.descriptor_buffer());
	out_pipeline.bind(set0);
}

std::span<glm::mat4x4 const> SceneRenderer::make_instance_mats(Node const& node, glm::mat4x4 const& parent) {
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

void SceneRenderer::render(Renderer& renderer, vk::CommandBuffer cb, Skybox const& skybox) {
	auto state = m_scene->pipeline_state;
	state.depth_test = false;
	auto pipeline = renderer.bind_pipeline(cb, state, "skybox");
	pipeline.set_line_width(1.0f);
	update_view(pipeline);
	auto& set1 = pipeline.next_set(1);
	set1.update(0, skybox.cubemap().descriptor_image());
	pipeline.bind(set1);
	auto const mat = glm::translate(matrix_identity_v, m_scene->camera().transform.position());
	draw(cb, skybox.static_mesh(), {&mat, 1u});
}

void SceneRenderer::render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4 parent) {
	auto const& resources = m_scene->resources();
	if (auto mesh_id = node.find<Mesh>()) {
		static auto const s_default_material = Material{std::make_unique<LitMaterial>()};
		auto const& mesh = resources.meshes[*mesh_id];
		for (auto const& primitive : mesh.primitives) {
			auto const& material = primitive.material ? resources.materials[primitive.material->value()] : static_cast<Material const&>(s_default_material);
			auto const& static_mesh = resources.static_meshes[primitive.static_mesh];

			auto state = m_scene->pipeline_state;
			state.topology = static_mesh.topology();
			auto pipeline = renderer.bind_pipeline(cb, state, material.shader_id());
			pipeline.set_line_width(m_scene->pipeline_state.line_width);

			update_view(pipeline);
			auto const store = TextureStore{resources.textures, m_white, m_black};
			material.write_sets(pipeline, store);

			auto const mats = make_instance_mats(node, parent);
			draw(cb, static_mesh, mats);
		}
	}

	parent = parent * node.transform.matrix();
	for (auto const& id : node.children) { render(renderer, cb, m_scene->resources().nodes[id], parent); }
}

void SceneRenderer::draw(vk::CommandBuffer cb, StaticMesh const& static_mesh, std::span<glm::mat4x4 const> mats) {
	auto const instances = next_instances(mats);
	cb.bindVertexBuffers(1u, instances.buffer, vk::DeviceSize{0});
	static_mesh.draw(cb, mats.size());
	m_info.triangles_drawn += static_mesh.info().vertices / 3;
	++m_info.draw_calls;
}

BufferView SceneRenderer::next_instances(std::span<glm::mat4x4 const> mats) {
	auto& buffer = [&]() -> Buffer& {
		auto ret = Ptr<Buffer>{};
		if (m_instances.index < m_instances.buffers.size()) {
			ret = &m_instances.buffers[m_instances.index++];
		} else {
			++m_instances.index;
			ret = &m_instances.buffers.emplace_back(m_gfx, Buffer::Type::eInstance);
		}
		return *ret;
	}();
	buffer.write(mats);
	return buffer.view();
}
} // namespace facade
