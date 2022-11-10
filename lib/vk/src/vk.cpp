#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/vk/vk.hpp>
#include <algorithm>
#include <compare>
#include <span>

namespace facade {
namespace {
using namespace std::string_view_literals;

constexpr auto validation_layer_v = "VK_LAYER_KHRONOS_validation"sv;

vk::UniqueInstance create_instance(std::vector<char const*> extensions, std::uint32_t& out_flags) {
	auto dl = vk::DynamicLoader{};
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	if (out_flags & Vulkan::eValidation) {
		auto const available_layers = vk::enumerateInstanceLayerProperties();
		auto layer_search = [](vk::LayerProperties const& lp) { return lp.layerName == validation_layer_v; };
		if (std::find_if(available_layers.begin(), available_layers.end(), layer_search) != available_layers.end()) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} else {
			logger::warn("[Device] Validation layer requested but not found");
			out_flags &= ~Vulkan::eValidation;
		}
	}

	auto const version = VK_MAKE_VERSION(0, 1, 0);
	auto const ai = vk::ApplicationInfo{"gltf", version, "gltf", version, VK_API_VERSION_1_1};
	auto ici = vk::InstanceCreateInfo{};
	ici.pApplicationInfo = &ai;
	out_flags &= ~Vulkan::ePortability;
#if defined(__APPLE__)
	ici.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	out_flags |= Vulkan::ePortability;
#endif
	ici.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	ici.ppEnabledExtensionNames = extensions.data();
	if (out_flags & Vulkan::eValidation) {
		static constexpr char const* layers[] = {validation_layer_v.data()};
		ici.enabledLayerCount = 1;
		ici.ppEnabledLayerNames = layers;
	}
	auto ret = vk::UniqueInstance{};
	try {
		ret = vk::createInstanceUnique(ici);
	} catch (vk::LayerNotPresentError const& e) {
		logger::error("[Device] {}", e.what());
		ici.enabledLayerCount = 0;
		ret = vk::createInstanceUnique(ici);
	}
	VULKAN_HPP_DEFAULT_DISPATCHER.init(ret.get());
	return ret;
}

vk::UniqueDebugUtilsMessengerEXT make_debug_messenger(vk::Instance instance) {
	auto validationCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
								 VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*) -> vk::Bool32 {
		auto const msg = fmt::format("[vk] {}", pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN");
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			throw ValidationError{msg};
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: logger::warn("{}", msg); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logger::info("{}", msg); break;
		default: break;
		}
		return false;
	};

	auto dumci = vk::DebugUtilsMessengerCreateInfoEXT{};
	using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
	dumci.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo;
	using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
	dumci.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
	dumci.pfnUserCallback = validationCallback;
	return instance.createDebugUtilsMessengerEXTUnique(dumci, nullptr);
}

Gpu select_gpu(vk::Instance const instance, vk::SurfaceKHR const surface) {
	struct Entry {
		Gpu gpu{};
		int rank{};
		auto operator<=>(Entry const& rhs) const { return rank <=> rhs.rank; }
	};
	auto const get_queue_family = [surface](vk::PhysicalDevice const& device, std::uint32_t& out_family) {
		static constexpr auto queue_flags_v = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
		auto const properties = device.getQueueFamilyProperties();
		for (std::size_t i = 0; i < properties.size(); ++i) {
			auto const family = static_cast<std::uint32_t>(i);
			if (!device.getSurfaceSupportKHR(family, surface)) { continue; }
			if (!(properties[i].queueFlags & queue_flags_v)) { continue; }
			out_family = family;
			return true;
		}
		return false;
	};
	auto const devices = instance.enumeratePhysicalDevices();
	auto entries = std::vector<Entry>{};
	for (auto const& device : devices) {
		auto entry = Entry{.gpu = {device}};
		entry.gpu.properties = device.getProperties();
		if (!get_queue_family(device, entry.gpu.queue_family)) { continue; }
		if (entry.gpu.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { entry.rank -= 100; }
		entries.push_back(std::move(entry));
	}
	if (entries.empty()) { return {}; }
	std::sort(entries.begin(), entries.end());
	return std::move(entries.front().gpu);
}

vk::UniqueDevice create_device(std::span<char const* const> layers, Gpu const& gpu, std::uint32_t queue_family) {
	static constexpr float priority_v = 1.0f;
	static constexpr std::array required_extensions_v = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(__APPLE__)
		"VK_KHR_portability_subset",
#endif
	};

	auto qci = vk::DeviceQueueCreateInfo{{}, queue_family, 1, &priority_v};
	auto dci = vk::DeviceCreateInfo{};
	auto enabled = vk::PhysicalDeviceFeatures{};
	auto available = gpu.device.getFeatures();
	enabled.fillModeNonSolid = available.fillModeNonSolid;
	enabled.wideLines = available.wideLines;
	enabled.samplerAnisotropy = available.samplerAnisotropy;
	enabled.sampleRateShading = available.sampleRateShading;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
	dci.ppEnabledLayerNames = layers.data();
	dci.enabledExtensionCount = static_cast<std::uint32_t>(required_extensions_v.size());
	dci.ppEnabledExtensionNames = required_extensions_v.data();
	dci.pEnabledFeatures = &enabled;
	auto ret = gpu.device.createDeviceUnique(dci);
	if (ret) { VULKAN_HPP_DEFAULT_DISPATCHER.init(ret.get()); }
	return ret;
}
} // namespace

auto Vulkan::Instance::make(std::vector<char const*> extensions, std::uint32_t desired_flags) -> Instance {
	auto ret = Instance{};
	ret.instance = create_instance(std::move(extensions), desired_flags);
	if (!ret.instance) { throw InitError{"Failed to create Vulkan Instance"}; }
	ret.flags = desired_flags;
	if (ret.flags & eValidation) { ret.messenger = make_debug_messenger(*ret.instance); }
	return ret;
}

auto Vulkan::Device::make(Instance const& instance, vk::SurfaceKHR surface) -> Device {
	if (!surface) { throw InitError{"Invalid Vulkan surface"}; }
	auto ret = Device{};
	ret.gpu = select_gpu(*instance.instance, surface);
	if (!ret.gpu) { throw InitError{"Failed to find Vulkan Physical Device"}; }
	auto layers = std::vector<char const*>{};
	if (instance.flags & eValidation) { layers.push_back(validation_layer_v.data()); }
	ret.device = create_device(layers, ret.gpu, ret.gpu.queue_family);
	if (!ret.device) { throw InitError{"Failed to create Vulkan Device"}; }
	ret.queue = ret.device->getQueue(ret.gpu.queue_family, 0);
	return ret;
}

UniqueVma Vulkan::make_vma(vk::Instance instance, vk::PhysicalDevice gpu, vk::Device device) {
	assert(instance && gpu && device);
	auto vaci = VmaAllocatorCreateInfo{};
	vaci.instance = instance;
	vaci.physicalDevice = gpu;
	vaci.device = device;
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	auto vkFunc = VmaVulkanFunctions{};
	vkFunc.vkGetInstanceProcAddr = dl.vkGetInstanceProcAddr;
	vkFunc.vkGetDeviceProcAddr = dl.vkGetDeviceProcAddr;
	vaci.pVulkanFunctions = &vkFunc;
	auto ret = UniqueVma{};
	if (vmaCreateAllocator(&vaci, &ret.get().allocator) != VK_SUCCESS) { throw InitError{"Failed to create Vulkan Memory Allocator"}; }
	ret.get().device = device;
	return ret;
}

Vulkan::Vulkan(Wsi const& wsi, bool request_validation) noexcept(false) {
	auto flags = std::uint32_t{};
	if (request_validation) { flags |= eValidation; }
	instance = Vulkan::Instance::make(wsi.extensions(), flags);
	auto surface = wsi.make_surface(*instance.instance);
	device = Vulkan::Device::make(instance, *surface);
	vma = Vulkan::make_vma(*instance.instance, device.gpu.device, *device.device);
	shared = std::make_unique<Gfx::Shared>(*device.device, device.gpu.properties);
	logger::info("[Device] GPU: [{}] |", device.gpu.properties.deviceName.data());
}

Gfx Vulkan::gfx() const {
	return Gfx{
		.vma = vma,
		.instance = *instance.instance,
		.gpu = device.gpu.device,
		.device = *device.device,
		.queue_family = device.gpu.queue_family,
		.queue = device.queue,
		.shared = shared.get(),
	};
}
} // namespace facade
