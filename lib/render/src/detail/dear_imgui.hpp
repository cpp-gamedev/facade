#pragma once
#include <imgui.h>
#include <facade/glfw/glfw.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
class DearImGui {
  public:
	struct CreateInfo {
		Gfx gfx{};
		Glfw::Window window{};
		vk::RenderPass render_pass{};
		vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
		ColourSpace swapchain{ColourSpace::eSrgb};
	};

	DearImGui(CreateInfo const& info);
	~DearImGui();

	void new_frame();
	void end_frame();
	void render(vk::CommandBuffer cb);

	DearImGui& operator=(DearImGui&&) = delete;

  private:
	vk::UniqueDescriptorPool m_pool{};
};
} // namespace facade