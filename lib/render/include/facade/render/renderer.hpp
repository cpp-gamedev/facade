#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/render/gui.hpp>
#include <facade/util/colour_space.hpp>
#include <facade/vk/gfx.hpp>
#include <facade/vk/pipeline.hpp>
#include <facade/vk/shader.hpp>
#include <facade/vk/vertex_layout.hpp>
#include <memory>

namespace facade {
///
/// \brief Initialization data for constructing a Renderer.
///
struct RendererCreateInfo {
	std::size_t command_buffers{1};
	std::uint8_t desired_msaa{1};
};

struct RenderShader {
	Shader::Id vert{"lit.vert"};
	Shader::Id frag{"lit.frag"};
};

///
/// \brief Owns a Vulkan Swapchain and RenderPass, a Shader::Db and Pipes instance each, and renders to the Window passed in constructor.
///
class Renderer {
  public:
	using CreateInfo = RendererCreateInfo;

	///
	/// \brief Information about the Renderer state.
	///
	struct Info {
		vk::PresentModeKHR present_mode{};
		vk::SampleCountFlags supported_msaa{};
		vk::SampleCountFlagBits current_msaa{};
		ColourSpace colour_space{};
		std::size_t cbs_per_frame{};
	};

	///
	/// \brief Construct a Renderer instance.
	/// \param gfx Graphics context
	/// \param window Window to render to
	/// \param create_info Initialization data
	///
	Renderer(Gfx const& gfx, Glfw::Window window, Gui* gui, CreateInfo const& create_info = {});
	Renderer(Renderer&&) noexcept;
	Renderer& operator=(Renderer&&) noexcept;
	~Renderer() noexcept;

	///
	/// \brief Obtain the Info for the current Renderer state.
	/// \returns Info for the current Renderer state
	///
	Info info() const;
	///
	/// \brief Obtain the name of the GPU in use.
	/// \returns GPU name
	///
	std::string_view gpu_name() const;
	///
	/// \brief Obtain the current framebuffer extent (size).
	/// \returns Current framebuffer extent
	///
	glm::uvec2 framebuffer_extent() const;

	///
	/// \brief Check if a present mode is supported on the Swapchain in use.
	/// \param mode Desired present mode
	/// \returns false If not supported
	///
	bool is_supported(vk::PresentModeKHR mode) const;
	///
	/// \brief Request for swapchain recreation on the next frame with a new desired present mode.
	/// \param desired Desired present mode
	/// \returns false If not supported
	///
	bool request_mode(vk::PresentModeKHR desired);
	///
	/// \brief Request for swapchain recreation on the next frame with a new desired colour space.
	/// \param desired Desired colour space
	///
	void request_colour_space(ColourSpace desired);

	///
	/// \brief Acquire a swapchain image, wait for its drawn fence, refresh RenderTarget and Framebuffer.
	/// \param out Span of Vulkan Command Buffers to populate for drawing
	/// \returns false If acquiring the image fails (unlikely)
	///
	/// out.size() must be less than or equal to the command buffer count passed during construction.
	///
	bool next_frame(std::span<vk::CommandBuffer> out);
	///
	/// \brief Get or create a Vulkan Pipeline, bind it, and return a corresponding Pipeline.
	/// \param cb Command buffer to use to bind pipeline
	/// \param state Pipeline state to use to find / create a Vulkan Pipeline
	/// \param shader_id Shader Id to use to find / create a Vulkan Pipeline
	/// \returns Pipeline with corresponding descriptor sets to write to
	///
	Pipeline bind_pipeline(vk::CommandBuffer cb, VertexLayout const& vlayout, Pipeline::State state = {}, RenderShader shader = {});
	///
	/// \brief Execute render pass and submit all recorded command buffers to the graphics queue.
	/// \returns false If Swapchain Image has not been acquired
	///
	bool render();

	///
	/// \brief Add a Shader to the Db.
	/// \param id The id to associate the shader with
	/// \param spir_v SPIR-V shader code
	/// \returns Shader referencing added SpirV
	///
	Shader add_shader(std::string id, SpirV spir_v);
	///
	/// \brief Add a Shader to the Db.
	/// \param shader Shader data to add (SpirV is not owned by Renderer)
	/// \returns false If id or code are invalid / empty
	///
	bool add_shader(Shader shader);
	///
	/// \brief Find a Shader in the Db by its Id
	/// \param id Shader Id to search for
	/// \returns Shader if found, default / empty instance if not
	///
	Shader find_shader(std::string const& id) const;

	Gfx const& gfx() const;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
