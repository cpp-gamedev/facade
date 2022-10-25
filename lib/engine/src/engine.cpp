#include <facade/engine/engine.hpp>
#include <facade/render/renderer.hpp>
#include <facade/vk/vk.hpp>

namespace facade {
namespace {
static constexpr std::size_t command_buffers_v{1};

UniqueWin make_window(glm::ivec2 extent, char const* title) {
	auto ret = Glfw::Window::make();
	glfwSetWindowTitle(ret.get(), title);
	glfwSetWindowSize(ret.get(), extent.x, extent.y);
	return ret;
}
} // namespace

struct Engine::Impl {
	UniqueWin window;
	Vulkan vulkan;
	Gfx gfx;
	Renderer renderer;

	Impl(CreateInfo const& info)
		: window(make_window(info.extent, info.title)), vulkan(GlfwWsi{window}), gfx{vulkan.gfx()},
		  renderer(gfx, window, Renderer::CreateInfo{command_buffers_v, info.msaa_samples}) {}
};

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;
Engine::~Engine() noexcept = default;

Engine::Engine(CreateInfo const& info) : m_impl(std::make_unique<Impl>(info)) {
	if (info.auto_show) { show_window(); }
}

bool Engine::add_shader(Shader shader) { return m_impl->renderer.add_shader(shader); }

void Engine::show_window() { glfwShowWindow(m_impl->window.get()); }

void Engine::hide_window() { glfwHideWindow(m_impl->window.get()); }

bool Engine::running() const { return !glfwWindowShouldClose(m_impl->window.get()); }

bool Engine::next_frame(vk::CommandBuffer& out) {
	m_impl->window.get().glfw->poll_events();
	if (!m_impl->renderer.next_frame({&out, 1})) { return false; }
	return true;
}

void Engine::submit() { m_impl->renderer.render(); }

void Engine::request_stop() { glfwSetWindowShouldClose(m_impl->window.get(), GLFW_TRUE); }

Gfx const& Engine::gfx() const { return m_impl->gfx; }
Glfw::Window Engine::window() const { return m_impl->window; }
Input const& Engine::input() const { return m_impl->window.get().state().input; }
Renderer& Engine::renderer() const { return m_impl->renderer; }
} // namespace facade
