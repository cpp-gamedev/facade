#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
class Renderer;

class Gui {
  public:
	virtual ~Gui() = default;

	virtual void init(Renderer const& renderer) = 0;

	virtual void new_frame() = 0;
	virtual void end_frame() = 0;
	virtual void render(vk::CommandBuffer cb) = 0;
};
} // namespace facade
