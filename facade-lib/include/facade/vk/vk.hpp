#pragma once
#include <facade/vk/gfx.hpp>
#include <facade/vk/vma.hpp>
#include <facade/vk/wsi.hpp>

namespace facade {
struct Gpu {
	vk::PhysicalDevice device{};
	vk::PhysicalDeviceProperties properties{};
	std::uint32_t queue_family{};

	explicit operator bool() const { return !!device; }
};

class Vulkan {
  public:
	enum Flag : std::uint32_t {
		eValidation = 1 << 0,
		ePortability = 1 << 1,
	};

	struct Instance {
		vk::UniqueInstance instance{};
		vk::UniqueDebugUtilsMessengerEXT messenger{};
		std::uint32_t flags{};

		static Instance make(std::vector<char const*> extensions, std::uint32_t desired_flags);
	};

	struct Device {
		Gpu gpu{};
		vk::UniqueDevice device{};
		vk::Queue queue{};

		static Device make(Instance const& instance, vk::SurfaceKHR surface);
	};

	static UniqueVma make_vma(vk::Instance instance, vk::PhysicalDevice gpu, vk::Device device);

	Vulkan(Wsi const& wsi) noexcept(false);

	Gfx gfx() const;
	Gpu const& gpu() const { return device.gpu; }

  private:
	Instance instance{};
	Device device{};
	UniqueVma vma{};
	std::unique_ptr<Gfx::Shared> shared{};
};
} // namespace facade
