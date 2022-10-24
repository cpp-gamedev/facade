#include <facade/vk/swapchain.hpp>
#include <cassert>
#include <limits>
#include <span>

namespace facade {
namespace {
SurfaceFormats make_surface_formats(std::span<vk::SurfaceFormatKHR const> available) {
	auto ret = SurfaceFormats{};
	for (auto const format : available) {
		if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			if (std::find(std::begin(srgb_formats_v), std::end(srgb_formats_v), format.format) != std::end(srgb_formats_v)) {
				ret.srgb.push_back(format.format);
			} else {
				ret.linear.push_back(format.format);
			}
		}
	}
	return ret;
}

vk::SurfaceFormatKHR surface_format(SurfaceFormats const& formats, ColourSpace colour_space) {
	if (colour_space == ColourSpace::eSrgb && !formats.srgb.empty()) { return formats.srgb.front(); }
	return formats.linear.front();
}

constexpr std::uint32_t image_count(vk::SurfaceCapabilitiesKHR const& caps) noexcept {
	if (caps.maxImageCount < caps.minImageCount) { return std::max(3U, caps.minImageCount); }
	return std::clamp(3U, caps.minImageCount, caps.maxImageCount);
}

constexpr vk::Extent2D image_extent(vk::SurfaceCapabilitiesKHR const& caps, vk::Extent2D const fb) noexcept {
	constexpr auto limitless_v = std::numeric_limits<std::uint32_t>::max();
	if (caps.currentExtent.width < limitless_v && caps.currentExtent.height < limitless_v) { return caps.currentExtent; }
	auto const x = std::clamp(fb.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	auto const y = std::clamp(fb.height, caps.minImageExtent.height, caps.maxImageExtent.height);
	return vk::Extent2D{x, y};
}

vk::SwapchainCreateInfoKHR make_swci(Gfx const& gfx, vk::SurfaceKHR surface, SurfaceFormats const& formats, ColourSpace colour_space, vk::PresentModeKHR mode) {
	vk::SwapchainCreateInfoKHR ret;
	auto const format = surface_format(formats, colour_space);
	ret.surface = surface;
	ret.presentMode = mode;
	ret.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	ret.queueFamilyIndexCount = 1U;
	ret.pQueueFamilyIndices = &gfx.queue_family;
	ret.imageColorSpace = format.colorSpace;
	ret.imageArrayLayers = 1U;
	ret.imageFormat = format.format;
	return ret;
}

constexpr bool is_zero(glm::uvec2 const extent) { return extent.x == 0 || extent.y == 0; }
} // namespace

Swapchain::Swapchain(Gfx const& gfx, vk::UniqueSurfaceKHR surface, ColourSpace colour_space) : m_gfx{gfx}, m_surface{std::move(surface)} {
	m_formats = make_surface_formats(m_gfx.gpu.getSurfaceFormatsKHR(*m_surface));
	auto const supported_modes = m_gfx.gpu.getSurfacePresentModesKHR(*m_surface);
	m_supported_modes = {supported_modes.begin(), supported_modes.end()};
	info = make_swci(m_gfx, *m_surface, m_formats, colour_space, vk::PresentModeKHR::eFifo);
}

vk::Result Swapchain::refresh(Spec const& spec) {
	auto create_info = info;
	if (spec.colour_space) { create_info.imageFormat = surface_format(m_formats, *spec.colour_space).format; }
	if (spec.mode) {
		assert(m_supported_modes.contains(*spec.mode));
		create_info.presentMode = *spec.mode;
	}
	auto const caps = m_gfx.gpu.getSurfaceCapabilitiesKHR(*m_surface);
	create_info.imageExtent = image_extent(caps, to_vk_extent(spec.extent));
	create_info.minImageCount = image_count(caps);
	create_info.oldSwapchain = m_storage.swapchain.get();
	auto swapchain = vk::SwapchainKHR{};
	auto const ret = m_gfx.device.createSwapchainKHR(&create_info, nullptr, &swapchain);
	if (ret != vk::Result::eSuccess) { return ret; }
	info = std::move(create_info);
	m_gfx.shared->defer_queue.push(std::move(m_storage));
	m_storage.swapchain = vk::UniqueSwapchainKHR{swapchain, m_gfx.device};
	auto const images = m_gfx.device.getSwapchainImagesKHR(*m_storage.swapchain);
	m_storage.images.clear();
	m_storage.views.clear();
	for (auto const image : images) {
		m_storage.views.insert(m_gfx.vma.make_image_view(image, info.imageFormat));
		m_storage.images.insert({image, *m_storage.views.span().back(), info.imageExtent});
	}
	return ret;
}

vk::Result Swapchain::acquire(glm::uvec2 extent, ImageView& out, vk::Semaphore sempahore, vk::Fence fence) {
	if (m_storage.acquired || is_zero(extent)) { return vk::Result::eNotReady; }
	auto index = std::uint32_t{};
	auto const ret = m_gfx.device.acquireNextImageKHR(*m_storage.swapchain, std::numeric_limits<std::uint64_t>::max(), sempahore, fence, &index);
	if (ret != vk::Result::eSuccess) {
		if (ret == vk::Result::eErrorOutOfDateKHR || ret == vk::Result::eSuboptimalKHR) { refresh(Spec{.extent = extent}); }
		return ret;
	}
	auto const images = m_storage.images.span();
	assert(index < images.size());
	out = images[index];
	m_storage.acquired = index;
	return ret;
}

vk::Result Swapchain::present(glm::uvec2 extent, vk::Semaphore wait) {
	if (!m_storage.acquired || is_zero(extent)) { return vk::Result::eNotReady; }
	auto pi = vk::PresentInfoKHR{};
	pi.swapchainCount = 1U;
	pi.pSwapchains = &*m_storage.swapchain;
	pi.waitSemaphoreCount = 1U;
	pi.pWaitSemaphores = &wait;
	pi.pImageIndices = &*m_storage.acquired;
	auto const ret = m_gfx.queue.presentKHR(&pi);
	m_storage.acquired.reset();
	if (ret == vk::Result::eErrorOutOfDateKHR || ret == vk::Result::eSuboptimalKHR) { refresh(Spec{.extent = extent}); }
	return ret;
}
} // namespace facade
