#pragma once
#include <facade/util/ptr.hpp>
#include <facade/vk/set_allocator.hpp>
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

	explicit operator bool() const { return m_pipeline && m_pool; }

  private:
	Pipeline(vk::Pipeline p, vk::PipelineLayout l, Ptr<SetAllocator::Pool> pool, Ptr<vk::PhysicalDeviceLimits const> limits)
		: m_pipeline{p}, m_layout{l}, m_pool{pool}, m_limits(limits) {}

	vk::Pipeline m_pipeline{};
	vk::PipelineLayout m_layout{};
	vk::CommandBuffer m_cb{};
	Ptr<SetAllocator::Pool> m_pool{};
	Ptr<vk::PhysicalDeviceLimits const> m_limits{};

	friend class Pipes;
};
} // namespace facade
