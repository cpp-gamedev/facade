#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/util/time.hpp>
#include <facade/vk/shader.hpp>

namespace facade {
struct Gfx;
class Renderer;
class Scene;
struct Window;

struct EngineCreateInfo {
	glm::uvec2 extent{1280, 800};
	char const* title{"facade"};
	std::uint8_t msaa_samples{2};
	bool auto_show{false};
};

class Engine {
  public:
	using CreateInfo = EngineCreateInfo;

	Engine(Engine&&) noexcept;
	Engine& operator=(Engine&&) noexcept;
	~Engine() noexcept;

	explicit Engine(CreateInfo const& info = {});

	bool add_shader(Shader shader);
	void show(bool reset_dt);
	void hide();

	bool running() const;
	float next_frame();
	void render(Scene& scene) const;
	void request_stop();

	void reload(CreateInfo const& info);

	Glfw::Window window() const;
	Gfx const& gfx() const;
	Input const& input() const;
	Renderer const& renderer() const;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
