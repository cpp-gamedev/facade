#include <facade/vk/render_frame.hpp>

namespace facade {
namespace {
vk::UniqueFramebuffer make_framebuffer(vk::Device device, vk::RenderPass rp, RenderTarget const& target) {
	auto fbci = vk::FramebufferCreateInfo{};
	fbci.renderPass = rp;
	auto const attachments = target.attachments();
	auto const span = attachments.span();
	fbci.attachmentCount = static_cast<std::uint32_t>(span.size());
	fbci.pAttachments = span.data();
	fbci.width = target.colour.extent.width;
	fbci.height = target.colour.extent.height;
	fbci.layers = 1;
	return device.createFramebufferUnique(fbci);
}

ImageBarrier undef_to_colour(ImageView const& image) {
	auto ret = ImageBarrier{};
	ret.barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
	ret.barrier.oldLayout = vk::ImageLayout::eUndefined;
	ret.barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
	ret.barrier.image = image.image;
	ret.barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	ret.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	return ret;
}

ImageBarrier undef_to_depth(ImageView const& image) {
	auto ret = ImageBarrier{};
	ret.barrier.srcAccessMask = ret.barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
	ret.barrier.oldLayout = vk::ImageLayout::eUndefined;
	ret.barrier.newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	ret.barrier.image = image.image;
	ret.barrier.subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
	ret.stages.first = ret.stages.second = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	return ret;
}

ImageBarrier colour_to_present(ImageView const& image) {
	auto ret = ImageBarrier{};
	ret.barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
	ret.barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
	ret.barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	ret.barrier.image = image.image;
	ret.barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	ret.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe};
	return ret;
}
} // namespace

void RenderTarget::to_draw(vk::CommandBuffer const cb) {
	if (colour.view) { undef_to_colour(colour).transition(cb); }
	if (resolve.view) { undef_to_colour(resolve).transition(cb); }
	if (depth.view) { undef_to_depth(depth).transition(cb); }
}

void RenderTarget::to_present(vk::CommandBuffer const cb) {
	if (resolve.view) {
		colour_to_present(resolve).transition(cb);
	} else {
		colour_to_present(colour).transition(cb);
	}
}

RenderFrame RenderFrame::make(Gfx const& gfx, std::size_t secondary) {
	auto ret = RenderFrame{
		.gfx = gfx,
		.sync =
			Sync{
				.draw = gfx.device.createSemaphoreUnique({}),
				.present = gfx.device.createSemaphoreUnique({}),
				.drawn = gfx.device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}),
			},
		.cmd_alloc = Cmd::Allocator::make(gfx, vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
	};
	ret.primary = ret.cmd_alloc.allocate(false);
	secondary = std::min(secondary, max_secondary_cmds_v);
	for (std::size_t i = 0; i < secondary; ++i) { ret.secondary.insert(ret.cmd_alloc.allocate(true)); }
	return ret;
}

Framebuffer RenderFrame::refresh(vk::RenderPass rp, RenderTarget const& rt) {
	framebuffer = make_framebuffer(gfx.device, rp, rt);
	return {rt.attachments(), *framebuffer, primary, secondary.span()};
}

void RenderFrame::submit(vk::Queue queue) const {
	static constexpr vk::PipelineStageFlags wait = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	auto si = vk::SubmitInfo{*sync.draw, wait, primary, *sync.present};
	queue.submit(si, *sync.drawn);
}
} // namespace facade
