#pragma once
#include <facade/engine/engine.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/time.hpp>

namespace facade {
struct ContextCreateInfo {
	glm::uvec2 extent{1280, 800};
	char const* title{"facade"};
	std::uint8_t msaa_samples{2};
	bool auto_show{false};
};

///
/// \brief Owns and operates a Window, Engine and Scene
///
class Context {
	// order is important here: window and engine must be initialized before / destroyed after all other data members
	UniqueWin m_window;
	Engine m_engine;

  public:
	using CreateInfo = ContextCreateInfo;

	Context(CreateInfo const& create_info = {});

	///
	/// \brief Register a shader for the renderer to look up during draws
	///
	void add_shader(Shader shader);

	///
	/// \brief Show the window
	///
	void show(bool reset_dt);
	///
	/// \brief Hide the window
	///
	void hide();

	///
	/// \brief Check if window has been requested to be closed
	///
	bool running() const { return !glfwWindowShouldClose(m_window.get()); }
	///
	/// \brief Submit previous frame (if applicable), and begin a new frame
	/// \returns time elapsed since the previous call to next_frame()
	///
	float next_frame();

	///
	/// \brief Request window to be closed
	///
	void request_stop();

	Glfw::Window const& window() const { return m_window; }
	Engine const& engine() const { return m_engine; }
	Glfw::State const& state() const { return m_window.get().state(); }
	Input const& input() const { return m_window.get().state().input; }

	///
	/// \brief The scene to render every frame
	///
	Scene scene;

  private:
	DeltaTime m_dt{};
	vk::CommandBuffer m_cb{};
	bool m_ready_to_render{};
};
} // namespace facade
