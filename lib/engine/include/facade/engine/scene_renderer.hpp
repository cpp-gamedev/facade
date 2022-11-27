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
	std::span<glm::mat4x4 const> make_instance_mats(Node const& node, glm::mat4x4 const& parent);

	void render(Renderer& renderer, vk::CommandBuffer cb, Skybox const& skybox);
	void render(Renderer& renderer, vk::CommandBuffer cb, Node const& node, glm::mat4 parent = matrix_identity_v);
	void draw(vk::CommandBuffer cb, StaticMesh const& static_mesh, std::span<glm::mat4x4 const> mats);

	BufferView next_instances(std::span<glm::mat4x4 const> mats);

	struct Instances {
		std::vector<Buffer> buffers{};
		std::size_t index{};

		void rotate();
	};

	Gfx m_gfx;
	Instances m_instances{};
	Sampler m_sampler;
	Buffer m_view_proj;
	Buffer m_dir_lights;
	Texture m_white;
	Texture m_black;
	Info m_info{};

	std::vector<glm::mat4x4> m_instance_mats{};
	Scene const* m_scene{};
};
} // namespace facade
