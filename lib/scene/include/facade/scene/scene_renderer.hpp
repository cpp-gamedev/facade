#pragma once
#include <facade/scene/scene.hpp>

namespace facade {
class SceneRenderer {
  public:
	explicit SceneRenderer(Gfx const& gfx);

	void render(Scene const& scene, Renderer& renderer, vk::CommandBuffer cb);

  private:
	void write_view(glm::vec2 const extent);
	void update_view(Pipeline& out_pipeline) const;
	std::span<glm::mat4x4 const> make_instances(Node const& node, glm::mat4x4 const& parent);

	void render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4 const& parent = matrix_identity_v);

	Gfx m_gfx;
	Sampler m_sampler;
	Buffer m_view_proj;
	Buffer m_dir_lights;
	Texture m_white;

	std::vector<glm::mat4x4> m_instance_mats{};
	Scene const* m_scene{};
};
} // namespace facade
