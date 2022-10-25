#include <facade/vk/render_pass.hpp>
#include <glm/vec2.hpp>

namespace facade {
namespace {
vk::UniqueRenderPass create_render_pass(vk::Device device, vk::SampleCountFlagBits samples, vk::Format colour, vk::Format depth) {
	auto attachments = std::vector<vk::AttachmentDescription>{};
	auto colour_ref = vk::AttachmentReference{};
	auto depth_ref = vk::AttachmentReference{};
	auto resolve_ref = vk::AttachmentReference{};

	auto subpass = vk::SubpassDescription{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colour_ref;

	colour_ref.attachment = static_cast<std::uint32_t>(attachments.size());
	colour_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
	auto attachment = vk::AttachmentDescription{};
	attachment.format = colour;
	attachment.samples = samples;
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.storeOp = samples > vk::SampleCountFlagBits::e1 ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
	attachment.initialLayout = attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachments.push_back(attachment);

	if (depth != vk::Format::eUndefined) {
		subpass.pDepthStencilAttachment = &depth_ref;
		depth_ref.attachment = static_cast<std::uint32_t>(attachments.size());
		depth_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		auto attachment = vk::AttachmentDescription{};
		attachment.format = depth;
		attachment.samples = samples;
		attachment.loadOp = vk::AttachmentLoadOp::eClear;
		attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
		attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment.initialLayout = attachment.finalLayout = depth_ref.layout;
		attachments.push_back(attachment);
	}

	if (samples > vk::SampleCountFlagBits::e1) {
		subpass.pResolveAttachments = &resolve_ref;
		resolve_ref.attachment = static_cast<std::uint32_t>(attachments.size());
		resolve_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

		attachment.samples = vk::SampleCountFlagBits::e1;
		attachment.loadOp = vk::AttachmentLoadOp::eDontCare;
		attachment.storeOp = vk::AttachmentStoreOp::eStore;
		attachments.push_back(attachment);
	}

	auto rpci = vk::RenderPassCreateInfo{};
	rpci.attachmentCount = static_cast<std::uint32_t>(attachments.size());
	rpci.pAttachments = attachments.data();
	rpci.subpassCount = 1U;
	rpci.pSubpasses = &subpass;
	auto sd = vk::SubpassDependency{};
	sd.srcSubpass = VK_SUBPASS_EXTERNAL;
	sd.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	sd.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	sd.srcStageMask = sd.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	rpci.dependencyCount = 1U;
	rpci.pDependencies = &sd;
	return device.createRenderPassUnique(rpci);
}
} // namespace

RenderPass::RenderPass(Gfx const& gfx, vk::SampleCountFlagBits samples, vk::Format colour, vk::Format depth)
	: m_gfx{gfx}, m_render_pass{create_render_pass(gfx.vma.device, samples, colour, depth)}, m_samples(samples) {
	m_colour.image = m_gfx.shared->defer_queue;
	m_depth.image = m_gfx.shared->defer_queue;
	m_depth.format = depth;
	if (m_samples > vk::SampleCountFlagBits::e1) { m_colour.format = colour; }
}

RenderTarget RenderPass::refresh(ImageView c) {
	if (!m_gfx.device) { return {}; }
	auto ret = RenderTarget{};
	if (m_colour.format != vk::Format::eUndefined) {
		if (m_colour.image.get().get().extent != c.extent) {
			auto const info = ImageCreateInfo{
				.format = m_colour.format,
				.usage = vk::ImageUsageFlagBits::eColorAttachment,
				.aspect = vk::ImageAspectFlagBits::eColor,
				.samples = m_samples,
			};
			m_colour.image.swap(m_gfx.vma.make_image(info, c.extent));
		}
		ret.colour = m_colour.image.get().get().image_view();
		ret.resolve = c;
	} else {
		ret.colour = c;
	}
	if (m_depth.format != vk::Format::eUndefined) {
		if (m_depth.image.get().get().extent != c.extent) {
			auto const info = ImageCreateInfo{
				.format = m_depth.format,
				.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
				.aspect = vk::ImageAspectFlagBits::eDepth,
				.samples = m_samples,
			};
			m_depth.image.swap(m_gfx.vma.make_image(info, c.extent));
		}
		ret.depth = m_depth.image.get().get().image_view();
	}
	m_extent = ret.extent = c.extent;
	return ret;
}

void RenderPass::bind(vk::CommandBuffer cb, vk::Pipeline pipeline) {
	cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	glm::vec2 const extent = glm::uvec2{m_extent.width, m_extent.height};
	auto viewport = vk::Viewport{0.0f, extent.y, extent.x, -extent.y, 0.0f, 1.0f};
	auto scissor = vk::Rect2D{{}, m_extent};
	cb.setViewport(0U, viewport);
	cb.setScissor(0U, scissor);
}

void RenderPass::execute(Framebuffer const& frame, vk::ClearColorValue const& clear) {
	vk::ClearValue const clear_values[] = {clear, vk::ClearDepthStencilValue{1.0f, 0U}};
	auto rpbi = vk::RenderPassBeginInfo{*m_render_pass, frame.framebuffer, vk::Rect2D{{}, m_extent}, clear_values};
	frame.primary.beginRenderPass(rpbi, vk::SubpassContents::eSecondaryCommandBuffers);
	frame.primary.executeCommands(static_cast<std::uint32_t>(frame.secondary.size()), frame.secondary.data());
	frame.primary.endRenderPass();
}
} // namespace facade
