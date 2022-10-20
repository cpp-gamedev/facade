#pragma once
#include <facade/util/ring_buffer.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/render_target.hpp>

namespace facade {
struct RenderFrame {
	static constexpr std::size_t max_secondary_cmds_v{4};

	struct Sync {
		vk::UniqueSemaphore draw{};
		vk::UniqueSemaphore present{};
		vk::UniqueFence drawn{};
	};

	Gfx gfx{};
	Sync sync{};
	Cmd::Allocator cmd_alloc{};
	vk::UniqueFramebuffer framebuffer{};
	vk::CommandBuffer primary{};
	FlexArray<vk::CommandBuffer, max_secondary_cmds_v> secondary{};

	static RenderFrame make(Gfx const& gfx, std::size_t secondary);

	Framebuffer refresh(vk::RenderPass rp, RenderTarget const& rt);

	void submit(vk::Queue queue) const;
};

template <std::size_t Size = buffering_v>
using RenderFrames = RingBuffer<RenderFrame, Size>;

template <std::size_t Size = buffering_v>
RenderFrames<Size> make_render_frames(Gfx const& gfx, std::size_t secondary) {
	auto ret = RenderFrames<Size>{};
	for (auto& frame : ret.t) { frame = RenderFrame::make(gfx, secondary); }
	return ret;
}
} // namespace facade
