#include <facade/context/context.hpp>
#include <facade/render/renderer.hpp>

namespace facade {
namespace {
UniqueWin make_window(glm::ivec2 extent, char const* title) {
	auto ret = Glfw::Window::make();
	glfwSetWindowTitle(ret.get(), title);
	glfwSetWindowSize(ret.get(), extent.x, extent.y);
	return ret;
}
} // namespace

Context::Context(CreateInfo const& create_info) noexcept(false)
	: m_window(make_window(create_info.extent, create_info.title)), m_engine(m_window, create_info.validation, create_info.msaa_samples),
	  scene(m_engine.gfx()) {
	if (create_info.auto_show) { show(true); }
}

void Context::add_shader(Shader shader) { m_engine.renderer().add_shader(std::move(shader)); }

void Context::show(bool reset_dt) {
	glfwShowWindow(m_window.get());
	if (reset_dt) { m_dt = {}; }
}

void Context::hide() { glfwHideWindow(m_window.get()); }

void Context::request_stop() { glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE); }

float Context::next_frame() {
	if (m_ready_to_render) {
		if (m_cb) { scene.render(m_engine.renderer(), m_cb); }
		m_engine.submit();
	}
	m_window.get().glfw->poll_events();
	if (!m_engine.next_frame(m_cb)) { m_cb = vk::CommandBuffer{}; }
	m_ready_to_render = true;
	return m_dt();
}
} // namespace facade
