#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <facade/glfw/input.hpp>
#include <facade/util/unique.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

namespace facade {
struct Glfw {
	struct Window;
	struct Deleter;
	struct State;

	bool active{};

	std::vector<char const*> vk_extensions() const;
	void poll_events();
	void reset_dt();

	bool operator==(Glfw const&) const = default;
};

using UniqueWin = Unique<Glfw::Window, Glfw::Deleter>;

struct Glfw::State {
	Input input{};
	std::vector<std::string> file_drops{};
	float dt{};

	static std::string to_filename(std::string_view path);
};

struct Glfw::Window {
	std::shared_ptr<Glfw> glfw{};
	GLFWwindow* win{};

	static UniqueWin make();

	glm::uvec2 window_extent() const;
	glm::uvec2 framebuffer_extent() const;

	State const& state() const;

	operator GLFWwindow*() const { return win; }

	bool operator==(Window const& rhs) const { return win == rhs.win; }
};

struct Glfw::Deleter {
	void operator()(Glfw const& glfw) const;
	void operator()(Window const& window) const;
};
} // namespace facade
