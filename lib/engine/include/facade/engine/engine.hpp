#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/time.hpp>
#include <facade/vk/shader.hpp>

namespace facade {
struct Gfx;
class Renderer;
class Scene;
struct Window;

enum class Validation : std::uint8_t { eDefault, eForceOn, eForceOff };

struct EngineCreateInfo {
	glm::uvec2 extent{1280, 800};
	char const* title{"facade"};
	std::uint8_t desired_msaa{2};
	bool auto_show{false};
	Validation validation{Validation::eDefault};
};

///
/// \brief Owns a Vulkan and Renderer instance
///
/// Only one active instance is supported
///
class Engine {
  public:
	using CreateInfo = EngineCreateInfo;

	Engine(Engine&&) noexcept;
	Engine& operator=(Engine&&) noexcept;
	~Engine() noexcept;

	///
	/// \brief Check if an instance of Engine is active
	///
	static bool is_instance_active();

	///
	/// \brief Construct an Engine instance
	///
	/// Throws if an instance already exists
	///
	explicit Engine(CreateInfo const& create_info = {}) noexcept(false);

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
	bool running() const;
	///
	/// \brief Begin a new frame and obtain time elapsed since the previous one
	///
	float next_frame();
	///
	/// \brief Render the scene
	///
	void render();

	///
	/// \brief Request window to be closed
	///
	void request_stop();

	Scene& scene() const;
	Gfx const& gfx() const;
	Glfw::Window const& window() const;
	Glfw::State const& state() const;
	Input const& input() const;
	Renderer& renderer() const;

  private:
	struct Impl;
	inline static Impl const* s_instance{};
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
