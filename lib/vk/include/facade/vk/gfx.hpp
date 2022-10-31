#pragma once
#include <facade/vk/defer_queue.hpp>
#include <facade/vk/vma.hpp>

namespace facade {
struct Gfx {
	struct Shared {
		struct DeviceBlock {
			vk::Device device;
			~DeviceBlock() { device.waitIdle(); }
		};
		vk::PhysicalDeviceLimits device_limits{};
		DeferQueue defer_queue{};
		std::mutex mutex{};
		DeviceBlock block{};

		Shared(vk::Device device, vk::PhysicalDeviceProperties const& props) : device_limits(props.limits), block{device} {}
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
