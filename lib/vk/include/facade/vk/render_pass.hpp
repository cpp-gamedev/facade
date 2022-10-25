#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>
#include <facade/vk/render_target.hpp>

namespace facade {
class RenderPass {
  public:
	RenderPass(Gfx const& gfx, vk::SampleCountFlagBits samples, vk::Format colour, vk::Format depth);

	[[nodiscard]] RenderTarget refresh(ImageView colour);
	void bind(vk::CommandBuffer cb, vk::Pipeline pipeline);
	void execute(Framebuffer const& framebuffer, vk::ClearColorValue const& clear);

	vk::RenderPass render_pass() const { return *m_render_pass; }

  private:
	struct Target {
		Defer<UniqueImage> image{};
		vk::Format format{};
	};

	Gfx m_gfx{};
	vk::UniqueRenderPass m_render_pass{};
	vk::SampleCountFlagBits m_samples{};
	Target m_colour{};
	Target m_depth{};
	vk::Extent2D m_extent{};
};
} // namespace facade
