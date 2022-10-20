#pragma once
#include <facade/util/defer_queue.hpp>
#include <facade/vk/vma.hpp>

namespace facade {
struct Gfx {
	struct Shared {
		vk::PhysicalDeviceLimits device_limits{};
		DeferQueue defer_queue{};
		std::mutex mutex{};
	};

	Vma vma{};
	vk::Instance instance{};
	vk::PhysicalDevice gpu{};
	vk::Device device{};
	std::uint32_t queue_family{};
	vk::Queue queue{};
	Shared* shared{};

	vk::Result wait(vk::ArrayProxy<vk::Fence> const& fences) const;
	void reset(vk::Fence fence) const;
};
} // namespace facade
