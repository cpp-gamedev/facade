#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
///
/// \brief Abstract interface for GUI (concrete DearImGui authored and owned by Engine).
///
class Gui {
  public:
	struct InitInfo {
		Gfx gfx{};
		Glfw::Window window{};
		vk::RenderPass render_pass{};
		vk::SampleCountFlagBits msaa{};
		ColourSpace colour_space{};
	};

	virtual ~Gui() = default;

	virtual void init(InitInfo const& info) = 0;
	virtual void render(vk::CommandBuffer cb) = 0;
};
} // namespace facade
