#include <facade/glfw/glfw_wsi.hpp>
#include <facade/util/error.hpp>

namespace facade {
std::vector<char const*> GlfwWsi::extensions() const {
	if (!window) { return {}; }
	return window.glfw->vk_extensions();
}

vk::UniqueSurfaceKHR GlfwWsi::make_surface(vk::Instance const instance) const {
	if (!window || !instance) { return {}; }
	auto ret = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(instance, window.win, {}, &ret) != VK_SUCCESS) { throw InitError{"Failed to create Vulkan Surface"}; }
	return vk::UniqueSurfaceKHR{vk::SurfaceKHR{ret}, instance};
}
} // namespace facade
