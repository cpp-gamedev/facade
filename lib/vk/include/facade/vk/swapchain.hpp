#pragma once
#include <facade/util/colour_space.hpp>
#include <facade/util/flex_array.hpp>
#include <facade/vk/gfx.hpp>
#include <glm/vec2.hpp>
#include <optional>
#include <unordered_set>

namespace facade {
template <typename T>
constexpr vk::Extent2D to_vk_extent(glm::tvec2<T> extent) {
	return {static_cast<std::uint32_t>(extent.x), static_cast<std::uint32_t>(extent.y)};
}

///
/// \brief Supported swapchain / surface image formats, separated into sRGB vs linear
///
struct SurfaceFormats {
	std::vector<vk::Format> srgb{};
	std::vector<vk::Format> linear{};
};

///
/// \brief Abstraction over Vulkan Swapchain + its resources
///
/// ColourSpace determines the surface image format (sRGB vs linear)
///  - sRGB formats perform sRGB colour correction of fragment shader outputs in hardware (default)
///  - Linear formats perform no colour correction (not recommended)
///
class Swapchain {
  public:
	static constexpr std::size_t max_images_v{8};
	using Lock = std::scoped_lock<std::mutex>;

	struct Spec {
		glm::uvec2 extent{};

		std::optional<vk::PresentModeKHR> mode{};
		std::optional<ColourSpace> colour_space{};
	};

	static constexpr ColourSpace colour_space(vk::Format format) { return is_srgb(format) ? ColourSpace::eSrgb : ColourSpace::eLinear; }

	Swapchain(Gfx const& gfx, vk::UniqueSurfaceKHR surface, ColourSpace colour_space = ColourSpace::eSrgb);

	ColourSpace colour_space() const { return colour_space(info.imageFormat); }
	std::unordered_set<vk::PresentModeKHR> const& supported_present_modes() const { return m_supported_modes; }
	vk::Result refresh(Spec const& spec);

	[[nodiscard]] vk::Result acquire(Lock const& q_lock, glm::uvec2 extent, ImageView& out, vk::Semaphore semaphore, vk::Fence fence = {});
	vk::Result present(Lock const& q_lock, glm::uvec2 extent, vk::Semaphore wait);

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
	std::unordered_set<vk::PresentModeKHR> m_supported_modes{};
	SurfaceFormats m_formats{};
	Storage m_storage{};
};
} // namespace facade
