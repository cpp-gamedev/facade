#pragma once
#include <facade/vk/gfx.hpp>
#include <span>

namespace facade {
class Cmd {
  public:
	struct Allocator {
		static constexpr vk::CommandPoolCreateFlags flags_v = vk::CommandPoolCreateFlagBits::eTransient;

		static Allocator make(Gfx const& gfx, vk::CommandPoolCreateFlags flags = flags_v);

		vk::Device device{};
		vk::UniqueCommandPool pool{};

		vk::Result allocate(std::span<vk::CommandBuffer> out, bool secondary = false) const;
		vk::CommandBuffer allocate(bool secondary) const;
	};

	Cmd(Gfx const& gfx, vk::PipelineStageFlags wait = vk::PipelineStageFlagBits::eAllCommands);
	~Cmd();

	Cmd& operator=(Cmd&&) = delete;

	vk::CommandBuffer cb{};

  private:
	Gfx m_gfx{};
	Allocator m_allocator{};
	vk::PipelineStageFlags m_wait{};
	vk::UniqueFence m_fence{};
};
} // namespace facade
