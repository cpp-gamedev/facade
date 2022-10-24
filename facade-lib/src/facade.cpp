#include <facade/facade.hpp>

namespace facade {
Context::Context(Engine::CreateInfo const& create_info) : engine(create_info), scene(engine.gfx()) {}

void Context::add_shader(Shader shader) { engine.add_shader(std::move(shader)); }

void Context::show(bool reset_dt) {
	engine.show_window();
	if (reset_dt) { m_dt = {}; }
}

float Context::next_frame() {
	if (m_ready_to_render) {
		if (m_cb) { scene.render(engine.renderer(), m_cb); }
		engine.submit();
	}
	if (!engine.next_frame(m_cb)) { m_cb = vk::CommandBuffer{}; }
	m_ready_to_render = true;
	return m_dt();
}
} // namespace facade
