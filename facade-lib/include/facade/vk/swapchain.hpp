#pragma once
#include <facade/util/colour_space.hpp>
#include <facade/util/defer_queue.hpp>
#include <facade/util/flex_array.hpp>
#include <facade/vk/gfx.hpp>
#include <glm/vec2.hpp>
#include <optional>

namespace facade {
template <typename T>
constexpr vk::Extent2D to_vk_extent(glm::tvec2<T> extent) {
	return {static_cast<std::uint32_t>(extent.x), static_cast<std::uint32_t>(extent.y)};
}

class Swapchain {
  public:
	static constexpr std::size_t max_images_v{8};

	struct Spec {
		glm::uvec2 extent{};

		std::optional<vk::PresentModeKHR> mode{};
		std::optional<vk::Format> format{};
	};

	static constexpr ColourSpace colour_space(vk::Format format) { return is_srgb(format) ? ColourSpace::eSrgb : ColourSpace::eLinear; }

	Swapchain(Gfx const& gfx, vk::UniqueSurfaceKHR surface, vk::PresentModeKHR mode);

	vk::Result refresh(Spec const& spec);

	[[nodiscard]] vk::Result acquire(glm::uvec2 extent, ImageView& out, vk::Semaphore semaphore, vk::Fence fence = {});
	vk::Result present(glm::uvec2 extent, vk::Semaphore wait);

	vk::SwapchainCreateInfoKHR info{};

  private:
	struct Storage {
		FlexArray<ImageView, max_images_v> images{};
		FlexArray<vk::UniqueImageView, max_images_v> views{};
		vk::UniqueSwapchainKHR swapchain{};
		std::optional<std::uint32_t> acquired{};
	};

	Gfx m_gfx{};
	vk::UniqueSurfaceKHR m_surface{};
	std::vector<vk::SurfaceFormatKHR> m_formats{};
	Storage m_storage{};
};
} // namespace facade
