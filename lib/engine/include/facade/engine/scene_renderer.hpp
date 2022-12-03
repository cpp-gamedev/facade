#pragma once
#include <facade/render/renderer.hpp>
#include <facade/scene/scene.hpp>
#include <facade/vk/buffer.hpp>

namespace facade {
class Skybox;

class SceneRenderer {
  public:
	struct Info {
		std::uint32_t triangles_drawn{};
		std::uint32_t draw_calls{};
	};

	explicit SceneRenderer(Gfx const& gfx);

	Info const& info() const { return m_info; }
	void render(Scene const& scene, Ptr<Skybox const> skybox, Renderer& renderer, vk::CommandBuffer cb);

  private:
	void write_view(glm::vec2 const extent);
	void update_view(Pipeline& out_pipeline) const;
	BufferView make_instance_mats(std::span<Transform const> instances, glm::mat4x4 const& parent);
	DescriptorBuffer make_joint_mats(Skin const& skin, glm::mat4x4 const& parent);

	void render(Renderer& renderer, vk::CommandBuffer cb, Skybox const& skybox);
	void render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4x4 parent = matrix_identity_v);
	void draw(Renderer& renderer, vk::CommandBuffer cb, Node const& node, Mesh::Primitive const& primitive);
	void draw(vk::CommandBuffer cb, MeshPrimitive const& mesh, BufferView instances);

	Gfx m_gfx;
	Material m_material;
	Buffer::VecPool<glm::mat4x4> m_instances;
	Buffer::VecPool<glm::mat4x4> m_joints;
	Sampler m_sampler;
	Buffer m_view_proj;
	Buffer m_dir_lights;
	Texture m_white;
	Texture m_black;
	Info m_info{};

	Scene const* m_scene{};
};
} // namespace facade
