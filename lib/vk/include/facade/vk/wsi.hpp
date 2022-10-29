#pragma once
#include <vulkan/vulkan.hpp>

namespace facade {
///
/// \brief Pure virtual interface for Vulkan Window System Integration.
///
struct Wsi {
	///
	/// \brief Obtain the device extensions required by this Wsi
	/// \returns vector of C strings, each corresponding to a Vulkan device extension
	///
	virtual std::vector<char const*> extensions() const = 0;
	///
	/// \brief Make a new Vulkan Surface
	/// \param instance The Vulkan Instance to use
	/// \returns RAII Vulkan Surface (expected to be not-null)
	///
	virtual vk::UniqueSurfaceKHR make_surface(vk::Instance instance) const = 0;
};
} // namespace facade
