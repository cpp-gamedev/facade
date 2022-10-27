#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/vk/wsi.hpp>

namespace facade {
struct GlfwWsi : Wsi {
	Glfw::Window window{};

	GlfwWsi(Glfw::Window window) : window(window) {}

	std::vector<char const*> extensions() const final;
	vk::UniqueSurfaceKHR make_surface(vk::Instance instance) const final;
};
} // namespace facade
