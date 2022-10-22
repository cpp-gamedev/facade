#pragma once
#include <vulkan/vulkan.hpp>

namespace facade {
struct Wsi {
	virtual std::vector<char const*> extensions() const = 0;
	virtual vk::UniqueSurfaceKHR make_surface(vk::Instance instance) const = 0;
};
} // namespace facade
