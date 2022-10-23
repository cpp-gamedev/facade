#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/gfx.hpp>
#include <facade/vk/pipeline.hpp>
#include <facade/vk/shader.hpp>
#include <memory>

namespace facade {
struct FrameStats {
	///
	/// \brief Total frames so far
	///
	std::uint64_t frame_counter{};
	///
	/// \brief Triangles drawn in previous frame
	///
	std::uint64_t triangles{};
	///
	/// \brief Draw calls in previous frame
	///
	std::uint32_t draw_calls{};
	///
	/// \brief Framerate (until previous frame)
	///
	std::uint32_t fps{};
};

struct RendererCreateInfo {
	std::size_t command_buffers{1};
	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
};

class Renderer {
  public:
	using CreateInfo = RendererCreateInfo;

	struct Info {
		vk::PresentModeKHR mode{};
		vk::SampleCountFlagBits samples{};
		ColourSpace colour_space{};
		std::size_t cbs_per_frame{};
	};

	Renderer(Gfx gfx, Glfw::Window window, CreateInfo const& info = {});
	Renderer(Renderer&&) noexcept;
	Renderer& operator=(Renderer&&) noexcept;
	~Renderer() noexcept;

	Info info() const;
	glm::uvec2 framebuffer_extent() const;
	FrameStats const& frame_stats() const;

	bool is_supported(vk::PresentModeKHR mode) const;
	bool request_mode(vk::PresentModeKHR desired) const;
	void request_colour_space(ColourSpace desired) const;

	bool next_frame(std::span<vk::CommandBuffer> out);
	Pipeline bind_pipeline(vk::CommandBuffer cb, Pipeline::State const& state = {}, std::string const& shader_id = "default");
	bool render();

	void draw(Pipeline& pipeline, StaticMesh const& mesh, std::span<glm::mat4x4 const> instances);

	Shader add_shader(std::string id, SpirV vert, SpirV frag);
	bool add_shader(Shader shader);
	Shader find_shader(std::string const& id) const;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
