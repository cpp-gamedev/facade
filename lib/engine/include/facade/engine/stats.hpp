#pragma once
#include <vulkan/vulkan.hpp>
#include <string_view>

namespace facade {
///
/// \brief Stores various statistics.
///
struct Stats {
	///
	/// \brief Total frames so far.
	///
	std::uint64_t frame_counter{};

	///
	/// \brief Triangles drawn in previous frame.
	///
	std::uint64_t triangles{};

	///
	/// \brief Draw calls in previous frame.
	///
	std::uint32_t draw_calls{};

	///
	/// \brief Framerate (until previous frame).
	///
	std::uint32_t fps{};

	///
	/// \brief Current present mode.
	///
	vk::PresentModeKHR mode{};

	///
	/// \brief Multi-sampled anti-aliasing level.
	///
	vk::SampleCountFlagBits current_msaa{};
	///
	/// \brief Supported anti-aliasing levels.
	///
	vk::SampleCountFlags supported_msaa{};
};
} // namespace facade
