#pragma once
#include <imgui.h>
#include <facade/glfw/glfw.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
class DearImgui {
  public:
	struct Info {
		Gfx gfx{};
		Glfw::Window window{};
		vk::RenderPass render_pass{};
		vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
		ColourSpace swapchain{ColourSpace::eSrgb};
	};

	DearImgui(Info const& info);
	~DearImgui();

	void new_frame();
	void end_frame();
	void render(vk::CommandBuffer cb);

	DearImgui& operator=(DearImgui&&) = delete;

  private:
	vk::UniqueDescriptorPool m_pool{};
};
} // namespace facade
