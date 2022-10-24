#include <facade/vk/cmd.hpp>

namespace facade {
auto Cmd::Allocator::make(Gfx const& gfx, vk::CommandPoolCreateFlags flags) -> Allocator {
	auto cpci = vk::CommandPoolCreateInfo{flags, gfx.queue_family};
	auto ret = Allocator{};
	ret.pool = gfx.device.createCommandPoolUnique(cpci);
	ret.device = gfx.device;
	return ret;
}

vk::Result Cmd::Allocator::allocate(std::span<vk::CommandBuffer> out, bool secondary) const {
	auto const level = secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
	auto cbai = vk::CommandBufferAllocateInfo{*pool, level, static_cast<std::uint32_t>(out.size())};
	return device.allocateCommandBuffers(&cbai, out.data());
}

vk::CommandBuffer Cmd::Allocator::allocate(bool secondary) const {
	auto ret = vk::CommandBuffer{};
	allocate({&ret, 1}, secondary);
	return ret;
}

Cmd::Cmd(Gfx const& gfx, vk::PipelineStageFlags wait) : m_gfx{gfx}, m_allocator{Allocator::make(gfx)}, m_wait{wait} {
	cb = m_allocator.allocate(false);
	cb.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

Cmd::~Cmd() {
	cb.end();
	m_fence = m_gfx.device.createFenceUnique({});
	auto si = vk::SubmitInfo{};
	si.pWaitDstStageMask = &m_wait;
	si.commandBufferCount = 1U;
	si.pCommandBuffers = &cb;
	{
		auto lock = std::scoped_lock{m_gfx.shared->mutex};
		if (m_gfx.queue.submit(1U, &si, *m_fence) != vk::Result::eSuccess) { return; }
	}
	m_gfx.wait(*m_fence);
}
} // namespace facade
