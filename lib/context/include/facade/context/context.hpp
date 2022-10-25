#pragma once
#include <facade/engine/engine.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/time.hpp>

namespace facade {
class Context {
  public:
	Context(Engine::CreateInfo const& create_info = {});

	void add_shader(Shader shader);

	void show(bool reset_dt);

	bool running() const { return engine.running(); }
	float next_frame();

	void request_stop() { engine.request_stop(); }

	Engine engine;
	Scene scene;

  private:
	DeltaTime m_dt{};
	vk::CommandBuffer m_cb{};
	bool m_ready_to_render{};
};
} // namespace facade
