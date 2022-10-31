#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/vk/wsi.hpp>

namespace facade {
///
/// \brief Concrete Wsi using GLFW.
///
struct GlfwWsi : Wsi {
	///
	/// \brief The window in use.
	///
	Glfw::Window window{};

	explicit GlfwWsi(Glfw::Window window) : window(window) {}

	std::vector<char const*> extensions() const final;
	vk::UniqueSurfaceKHR make_surface(vk::Instance instance) const final;
};
} // namespace facade
