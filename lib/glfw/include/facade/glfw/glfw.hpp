#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <facade/glfw/input.hpp>
#include <facade/util/unique.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

namespace facade {
///
/// \brief Represents a GLFW instance.
///
/// GLFW is initialized and shutdown agnostic of any windows created.
/// GLFW polls all windows for events simultaneously (ergo, they all share the same delta-time).
/// The required Vulkan Device extensions are specific to a platform, independent of any windows in use.
/// Glfw handles all these responsibilities.
///
struct Glfw {
	struct Window;
	struct Deleter;
	struct State;

	///
	/// \brief Whether this instance is active.
	///
	/// Used for move semantics.
	///
	bool active{};

	///
	/// \brief Obtain the Vulkan Device extensions for this windowing platform.
	///
	std::vector<char const*> vk_extensions() const;
	///
	/// \brief Poll all windows for events.
	///
	void poll_events();
	///
	/// \brief Reset delta time.
	///
	void reset_dt();

	bool operator==(Glfw const&) const = default;
};

///
/// \brief RAII GLFW Window
///
using UniqueWin = Unique<Glfw::Window, Glfw::Deleter>;

///
/// \brief A snapshot of the current window / input state.
///
struct Glfw::State {
	///
	/// \brief Input state.
	///
	Input input{};
	///
	/// \brief Paths of files dropped on the window this frame.
	///
	/// All paths are supplied in generic form (/path/to/file).
	///
	std::vector<std::string> file_drops{};
	///
	/// \brief Delta-time between polls.
	///
	float dt{};

	///
	/// \brief Obtain the filename of a file drop path.
	/// \param path Path to extract filename from
	///
	/// Passed path must be in generic form.
	///
	static std::string to_filename(std::string_view path);
};

///
/// \brief Represents a GLFW Window.
///
struct Glfw::Window {
	///
	/// \brief Shared Glfw instance.
	///
	std::shared_ptr<Glfw> glfw{};
	///
	/// \brief GLFW window associated with this instance.
	///
	GLFWwindow* win{};

	///
	/// \brief Create a new window.
	/// \returns RAII Glfw::Window
	///
	static UniqueWin make();

	///
	/// \brief Obtain the extent (size) of the window.
	///
	glm::uvec2 window_extent() const;
	///
	/// \brief Obtain the extent (size) of the framebuffer.
	///
	glm::uvec2 framebuffer_extent() const;

	///
	/// \brief Obtain the state since the last poll.
	///
	State const& state() const;

	operator GLFWwindow*() const { return win; }

	bool operator==(Window const& rhs) const { return win == rhs.win; }
};

struct Glfw::Deleter {
	void operator()(Glfw const& glfw) const;
	void operator()(Window const& window) const;
};
} // namespace facade
