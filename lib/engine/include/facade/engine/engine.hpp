#pragma once
#include <facade/glfw/glfw.hpp>
#include <facade/vk/shader.hpp>

namespace facade {
struct Gfx;
class Renderer;
class Scene;
struct Window;

///
/// \brief Owns a Vulkan and Renderer instance
///
/// Only one active instance is supported
///
class Engine {
  public:
	enum class Validation : std::uint8_t { eDefault, eForceOn, eForceOff };

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
	explicit Engine(Glfw::Window window, Validation validation = Validation::eDefault, std::uint8_t desired_msaa = 2) noexcept(false);

	///
	/// \brief Begin next frame's render pass
	///
	bool next_frame(vk::CommandBuffer& out);
	///
	/// \brief Submit render pass
	///
	void submit();

	Gfx const& gfx() const;
	Renderer& renderer() const;

  private:
	struct Impl;
	inline static Impl const* s_instance{};
	std::unique_ptr<Impl> m_impl{};
};
} // namespace facade
