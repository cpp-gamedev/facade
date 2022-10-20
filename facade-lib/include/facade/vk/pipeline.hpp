#pragma once
#include <facade/vk/set_allocator.hpp>
#include <facade/vk/static_mesh.hpp>
#include <glm/mat4x4.hpp>

namespace facade {
class Pipeline {
  public:
	struct State {
		vk::PolygonMode mode{vk::PolygonMode::eFill};
		vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
		bool depth_test{true};

		bool operator==(State const&) const = default;
	};

	void bind(vk::CommandBuffer cb);

	[[nodiscard]] DescriptorSet& next_set(std::uint32_t number) const;
	void bind(DescriptorSet const& set) const;
	void draw(StaticMesh const& mesh, std::span<glm::mat4x4 const> instances) const;

	explicit operator bool() const { return m_pipeline && m_pool; }

  private:
	Pipeline(vk::Pipeline p, vk::PipelineLayout l, SetAllocator::Pool* pool, class Drawer* d) : m_pipeline{p}, m_layout{l}, m_pool{pool}, m_drawer{d} {}

	vk::Pipeline m_pipeline{};
	vk::PipelineLayout m_layout{};
	vk::CommandBuffer m_cb{};
	SetAllocator::Pool* m_pool{};
	Drawer* m_drawer{};

	friend class Pipes;
};
} // namespace facade
