#include <detail/dear_imgui.hpp>
#include <facade/engine/engine.hpp>
#include <facade/render/renderer.hpp>
#include <facade/vk/vk.hpp>
#include <optional>

namespace facade {
namespace {
static constexpr std::size_t command_buffers_v{1};

UniqueWin make_window(glm::ivec2 extent, char const* title) {
	auto ret = Glfw::Window::make();
	glfwSetWindowTitle(ret.get(), title);
	glfwSetWindowSize(ret.get(), extent.x, extent.y);
	return ret;
}

struct GuiDearImGui : Gui {
	std::unique_ptr<DearImGui> imgui{};

	void init(InitInfo const& info) final {
		auto const dici = DearImGui::CreateInfo{
			.gfx = info.gfx,
			.window = info.window,
			.render_pass = info.render_pass,
			.samples = info.msaa,
			.swapchain = info.colour_space,
		};
		imgui = DearImGui::make(dici);
	}

	void new_frame() final {
		if (imgui) { imgui->new_frame(); }
	}

	void end_frame() final {
		if (imgui) { imgui->end_frame(); }
	}

	void render(vk::CommandBuffer cb) final {
		if (imgui) { imgui->render(cb); }
	}
};

struct RenderContext {
	struct Sub;

	UniqueWin window;
	Vulkan vulkan;
	Gfx gfx;
	Renderer renderer;
	std::uint8_t msaa;

	RenderContext(glm::uvec2 extent, char const* title, std::unique_ptr<Gui> gui, std::uint8_t msaa)
		: window(make_window(extent, title)), vulkan(GlfwWsi{window}), gfx{vulkan.gfx()},
		  renderer(gfx, window, std::move(gui), Renderer::CreateInfo{command_buffers_v, msaa}), msaa(msaa) {}
};

struct RenderContext::Sub {
	UniqueWin window;
	Renderer renderer;

	Sub(RenderContext const& main, glm::uvec2 extent, char const* title)
		: window(make_window(extent, title)), renderer(main.gfx, this->window, {}, Renderer::CreateInfo{command_buffers_v, main.msaa}) {}
};
} // namespace

struct Engine::Impl {
	RenderContext render_context;

	Impl(CreateInfo const& info) : render_context(info.extent, info.title, std::make_unique<GuiDearImGui>(), info.msaa_samples) {}
};

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;
Engine::~Engine() noexcept = default;

Engine::Engine(CreateInfo const& info) : m_impl(std::make_unique<Impl>(info)) {
	if (info.auto_show) { show_window(); }
}

bool Engine::add_shader(Shader shader) { return m_impl->render_context.renderer.add_shader(shader); }

void Engine::show_window() { glfwShowWindow(m_impl->render_context.window.get()); }

void Engine::hide_window() { glfwHideWindow(m_impl->render_context.window.get()); }

bool Engine::running() const { return !glfwWindowShouldClose(m_impl->render_context.window.get()); }

bool Engine::next_frame(vk::CommandBuffer& out) {
	m_impl->render_context.window.get().glfw->poll_events();
	if (!m_impl->render_context.renderer.next_frame({&out, 1})) { return false; }
	return true;
}

void Engine::submit() { m_impl->render_context.renderer.render(); }

void Engine::request_stop() { glfwSetWindowShouldClose(m_impl->render_context.window.get(), GLFW_TRUE); }

Gfx const& Engine::gfx() const { return m_impl->render_context.gfx; }
Glfw::Window Engine::window() const { return m_impl->render_context.window; }
Input const& Engine::input() const { return m_impl->render_context.window.get().state().input; }
Renderer& Engine::renderer() const { return m_impl->render_context.renderer; }
} // namespace facade
