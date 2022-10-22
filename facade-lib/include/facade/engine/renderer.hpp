#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/vk/gfx.hpp>
#include <facade/vk/pipeline.hpp>
#include <facade/vk/shader.hpp>
#include <facade/vk/swapchain.hpp>
#include <memory>

namespace facade {
struct RendererInfo {
	std::size_t command_buffers{1};
	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
};

class Renderer {
  public:
	using Info = RendererInfo;

	Renderer(Gfx gfx, Glfw::Window window, Info const& info = {});
	Renderer(Renderer&&) noexcept;
	Renderer& operator=(Renderer&&) noexcept;
	~Renderer() noexcept;

	glm::uvec2 framebuffer_extent() const;
	std::size_t command_buffers_per_frame() const;

	bool next_frame(std::span<vk::CommandBuffer> out);
	Pipeline bind_pipeline(vk::CommandBuffer cb, Pipeline::State const& state = {}, std::string const& shader_id = "default");
	bool render();

	Shader add_shader(std::string id, SpirV vert, SpirV frag);
	bool add_shader(Shader shader);
	Shader find_shader(std::string const& id) const;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
