#include <facade/engine/engine.hpp>
#include <facade/engine/renderer.hpp>
#include <facade/scene/scene.hpp>
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

constexpr auto get_samples(vk::SampleCountFlags supported, std::uint8_t desired) {
	if (desired >= 32 && (supported & vk::SampleCountFlagBits::e32)) { return vk::SampleCountFlagBits::e32; }
	if (desired >= 16 && (supported & vk::SampleCountFlagBits::e16)) { return vk::SampleCountFlagBits::e16; }
	if (desired >= 8 && (supported & vk::SampleCountFlagBits::e8)) { return vk::SampleCountFlagBits::e8; }
	if (desired >= 4 && (supported & vk::SampleCountFlagBits::e4)) { return vk::SampleCountFlagBits::e4; }
	if (desired >= 2 && (supported & vk::SampleCountFlagBits::e2)) { return vk::SampleCountFlagBits::e2; }
	return vk::SampleCountFlagBits::e1;
}

Renderer::CreateInfo make_renderer_info(Gpu const& gpu, std::uint8_t sample_count) {
	return Renderer::CreateInfo{command_buffers_v, get_samples(gpu.properties.limits.framebufferColorSampleCounts, sample_count)};
}
} // namespace

struct Engine::Impl {
	UniqueWin window;
	Vulkan vulkan;
	Gfx gfx;

	Renderer renderer;

	DeltaTime dt{};
	std::optional<vk::CommandBuffer> cb{};

	Impl(CreateInfo const& info)
		: window(make_window(info.extent, info.title)), vulkan(GlfwWsi{window}), gfx{vulkan.gfx()},
		  renderer(gfx, window, make_renderer_info(vulkan.gpu(), info.msaa_samples)) {}
};

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;
Engine::~Engine() noexcept = default;

Engine::Engine(CreateInfo const& info) : m_impl(std::make_unique<Impl>(info)) {
	if (info.auto_show) { show(true); }
}

bool Engine::add_shader(Shader shader) { return m_impl->renderer.add_shader(shader); }

void Engine::show(bool reset_dt) {
	glfwShowWindow(m_impl->window.get());
	if (reset_dt) { m_impl->dt = {}; }
}

void Engine::hide() { glfwHideWindow(m_impl->window.get()); }

bool Engine::running() const { return !glfwWindowShouldClose(m_impl->window.get()); }

float Engine::next_frame() {
	assert(!m_impl->cb);
	m_impl->window.get().glfw->poll_events();
	auto cb = vk::CommandBuffer{};
	if (!m_impl->renderer.next_frame({&cb, 1})) { return m_impl->dt(); }
	m_impl->cb = cb;
	return m_impl->dt();
}

void Engine::request_stop() { glfwSetWindowShouldClose(m_impl->window.get(), GLFW_TRUE); }

void Engine::render(Scene& scene) const {
	if (m_impl->cb) { scene.render(m_impl->renderer, *m_impl->cb); }
	m_impl->renderer.render();
	m_impl->cb.reset();
}

Gfx const& Engine::gfx() const { return m_impl->gfx; }
Glfw::Window Engine::window() const { return m_impl->window; }
Input const& Engine::input() const { return m_impl->window.get().state().input; }
Renderer const& Engine::renderer() const { return m_impl->renderer; }
} // namespace facade
