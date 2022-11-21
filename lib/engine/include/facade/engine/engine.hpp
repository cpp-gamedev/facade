#pragma once
#include <facade/build_version.hpp>
#include <facade/glfw/glfw.hpp>
#include <facade/scene/load_status.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/time.hpp>
#include <facade/util/unique_task.hpp>
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
	std::optional<std::uint32_t> force_thread_count{};
};

///
/// \brief Owns a Vulkan and Renderer instance
///
/// Only one active instance is supported
///
class Engine {
  public:
	using CreateInfo = EngineCreateInfo;
	using State = Glfw::State;

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
	/// \brief Poll events and obtain updated state
	///
	State const& poll();
	///
	/// \brief Render the scene
	///
	void render();

	///
	/// \brief Request window to be closed
	///
	void request_stop();

	///
	/// \brief Load a GLTF scene / Skybox asynchronously.
	///
	/// Subsequent requests will be rejected if one is in flight
	///
	bool load_async(std::string json_path, UniqueTask<void()> on_loaded = {});
	///
	/// \brief Obtain status of in-flight async load request (if active).
	///
	LoadStatus load_status() const;
	///
	/// \brief Obtain the number of loading threads.
	///
	std::size_t load_thread_count() const;

	glm::ivec2 window_position() const;
	glm::uvec2 window_extent() const;
	glm::uvec2 framebuffer_extent() const;

	Scene& scene() const;
	State const& state() const;
	Input const& input() const;
	Renderer& renderer() const;
	GLFWwindow* window() const;

  private:
	void update_load_request();

	struct Impl;
	inline static Impl const* s_instance{};
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
