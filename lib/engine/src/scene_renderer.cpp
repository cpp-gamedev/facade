#include <facade/engine/scene_renderer.hpp>
#include <facade/util/error.hpp>
#include <facade/util/zip_ranges.hpp>
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

constexpr vk::PrimitiveTopology to_primitive_topology(Topology topology) {
	switch (topology) {
	case Topology::ePoints: return vk::PrimitiveTopology::ePointList;
	case Topology::eLines: return vk::PrimitiveTopology::eLineList;
	case Topology::eLineStrip: return vk::PrimitiveTopology::eLineStrip;
	case Topology::eTriangles: return vk::PrimitiveTopology::eTriangleList;
	case Topology::eTriangleStrip: return vk::PrimitiveTopology::eTriangleStrip;
	case Topology::eTriangleFan: return vk::PrimitiveTopology::eTriangleFan;
	}
	throw Error{"Unsupported primitive topology: " + std::to_string(static_cast<int>(topology))};
}
} // namespace

SceneRenderer::SceneRenderer(Gfx const& gfx)
	: m_gfx(gfx), m_material(Material{LitMaterial{}, "default"}), m_instances(Buffer::Type::eInstance), m_joints(Buffer::Type::eStorage), m_sampler(gfx),
	  m_view_proj(gfx, Buffer::Type::eUniform), m_dir_lights(gfx, Buffer::Type::eStorage),
	  m_white(gfx, m_sampler.sampler(), Bmp1x1{0xff_B, 0xff_B, 0xff_B, 0xff_B}.view(), Texture::CreateInfo{.mip_mapped = false}),
	  m_black(gfx, m_sampler.sampler(), Bmp1x1{0x0_B, 0x0_B, 0x0_B, 0xff_B}.view(), Texture::CreateInfo{.mip_mapped = false}) {}

void SceneRenderer::render(Scene const& scene, Ptr<Skybox const> skybox, Renderer& renderer, vk::CommandBuffer cb) {
	m_scene = &scene;
	m_info = {};
	write_view(renderer.framebuffer_extent());
	if (skybox) { render(renderer, cb, *skybox); }
	for (auto const& node : m_scene->roots()) { render(renderer, cb, m_scene->resources().nodes[node]); }
	m_instances.rotate();
	m_joints.rotate();
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

BufferView SceneRenderer::make_instance_mats(std::span<Transform const> instances, glm::mat4x4 const& parent) {
	auto rewrite = [&](std::vector<glm::mat4x4>& mats) {
		if (instances.empty()) {
			mats.push_back(parent);
		} else {
			mats.reserve(instances.size());
			for (auto const& transform : instances) { mats.push_back(parent * transform.matrix()); }
		}
	};
	return m_instances.rewrite(m_gfx, instances.empty() ? 1u : instances.size(), rewrite).view();
}

DescriptorBuffer SceneRenderer::make_joint_mats(Skin const& skin, glm::mat4x4 const& parent) {
	auto const& resources = m_scene->resources();
	auto rewrite = [&](std::vector<glm::mat4x4>& mats) {
		for (auto const& [j, ibm] : zip_ranges(skin.joints, skin.inverse_bind_matrices)) {
			mats.push_back(parent * resources.nodes[j].transform.matrix() * ibm);
		}
	};
	return m_joints.rewrite(m_gfx, skin.joints.size(), rewrite).descriptor_buffer();
}

void SceneRenderer::render(Renderer& renderer, vk::CommandBuffer cb, Skybox const& skybox) {
	auto const& vlayout = skybox.mesh().vertex_layout();
	auto pipeline = renderer.bind_pipeline(cb, vlayout.input, {.depth_test = false}, {vlayout.shader, "skybox.frag"});
	pipeline.set_line_width(1.0f);
	update_view(pipeline);
	auto& set1 = pipeline.next_set(1);
	set1.update(0, skybox.cubemap().descriptor_image());
	pipeline.bind(set1);
	auto const mat = glm::translate(matrix_identity_v, m_scene->camera().transform.position());
	draw(cb, skybox.mesh(), make_instance_mats({}, mat));
}

Shader::Id frag_shader(Material const& mat) {
	if (std::holds_alternative<UnlitMaterial>(mat.instance)) { return "unlit.frag"; }
	return "lit.frag";
}

void SceneRenderer::render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4 parent) {
	auto const& resources = m_scene->resources();
	auto const store = TextureStore{resources.textures, m_white, m_black};
	parent = parent * node.transform.matrix();
	if (auto mesh_id = node.find<Mesh>()) {
		auto const& mesh = resources.meshes[*mesh_id];
		for (auto const& primitive : mesh.primitives) {
			auto const state = Pipeline::State{
				.mode = m_scene->render_mode.type == RenderMode::Type::eWireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill,
				.topology = to_primitive_topology(primitive.topology),
			};
			auto const& mesh_primitive = resources.primitives[primitive.primitive];
			VertexLayout const& vlayout = mesh_primitive.vertex_layout();
			auto const& material = primitive.material ? resources.materials[primitive.material->value()] : m_material;
			auto shader = RenderShader{vlayout.shader, frag_shader(material)};
			auto pipeline = renderer.bind_pipeline(cb, vlayout.input, state, shader);
			pipeline.set_line_width(m_scene->render_mode.line_width);
			update_view(pipeline);
			material.write_sets(pipeline, store);

			if (mesh_primitive.has_joints()) {
				auto& set3 = pipeline.next_set(3);
				set3.update(0, make_joint_mats(resources.skins[*node.find<Skin>()], parent));
				pipeline.bind(set3);
				draw(cb, mesh_primitive, {});
			} else {
				draw(cb, mesh_primitive, make_instance_mats(node.instances, parent));
			}
		}
	}

	for (auto const& id : node.children) { render(renderer, cb, m_scene->resources().nodes[id], parent); }
}

void SceneRenderer::draw(vk::CommandBuffer cb, MeshPrimitive const& mesh, BufferView instances) {
	if (instances.buffer) {
		cb.bindVertexBuffers(6u, instances.buffer, vk::DeviceSize{0});
		mesh.draw(cb, instances.count);
	} else {
		mesh.draw(cb, 1u);
	}
	m_info.triangles_drawn += mesh.info().vertices / 3;
	++m_info.draw_calls;
}
} // namespace facade
