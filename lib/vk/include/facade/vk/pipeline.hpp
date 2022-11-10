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
		float line_width{1.0f};
		bool depth_test{true};

		bool operator==(State const&) const = default;
	};

	void bind(vk::CommandBuffer cb);

	[[nodiscard]] DescriptorSet& next_set(std::uint32_t number) const;
	void set_line_width(float width) const;
	void bind(DescriptorSet const& set) const;
	void draw(StaticMesh const& mesh, std::span<glm::mat4x4 const> instances) const;

	explicit operator bool() const { return m_pipeline && m_pool; }

  private:
	Pipeline(vk::Pipeline p, vk::PipelineLayout l, SetAllocator::Pool* pool, vk::PhysicalDeviceLimits const* limits, class Drawer* d)
		: m_pipeline{p}, m_layout{l}, m_pool{pool}, m_limits(limits), m_drawer{d} {}

	vk::Pipeline m_pipeline{};
	vk::PipelineLayout m_layout{};
	vk::CommandBuffer m_cb{};
	SetAllocator::Pool* m_pool{};
	vk::PhysicalDeviceLimits const* m_limits{};
	Drawer* m_drawer{};

	friend class Pipes;
};
} // namespace facade
