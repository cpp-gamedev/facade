#include <facade/glfw/glfw.hpp>
#include <facade/util/error.hpp>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace facade {
namespace {
std::atomic<std::weak_ptr<Glfw>> g_glfw{};
std::mutex g_mutex{};

std::unordered_map<GLFWwindow*, Glfw::State> g_states{};

std::shared_ptr<Glfw> get_or_make_glfw() {
	if (auto glfw = g_glfw.load().lock()) { return glfw; }
	auto lock = std::scoped_lock{g_mutex};
	if (glfwInit() != GLFW_TRUE) { throw InitError{"GLFW initialization failed"}; }
	if (!glfwVulkanSupported()) {
		glfwTerminate();
		throw InitError{"Vulkan not supported"};
	}
	auto ret = std::make_shared<Glfw>();
	ret->active = true;
	g_glfw.load() = ret;
	return ret;
}

constexpr Action to_action(int glfw_action) {
	switch (glfw_action) {
	case GLFW_PRESS: return Action::ePress;
	case GLFW_REPEAT: return Action::eRepeat;
	case GLFW_RELEASE: return Action::eRelease;
	default: return Action::eNone;
	}
}
} // namespace

void Glfw::poll_events() {
	for (auto& [_, state] : g_states) {
		state.input.keyboard.next_frame();
		state.input.mouse.next_frame();
		state.file_drops.clear();
	}
	glfwPollEvents();
}

auto Glfw::Window::make() -> UniqueWin {
	auto ret = Window{.glfw = get_or_make_glfw()};
	if (!ret.glfw) { return {}; }
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	ret.win = glfwCreateWindow(1, 1, "[untitled]", nullptr, nullptr);
	if (!ret.win) { throw InitError{"GLFW window creation failed"}; }
	if (glfwRawMouseMotionSupported()) { glfwSetInputMode(ret, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE); }
	glfwSetKeyCallback(ret, [](GLFWwindow* w, int key, int, int action, int) { g_states[w].input.keyboard.on_key(key, to_action(action)); });
	glfwSetMouseButtonCallback(ret, [](GLFWwindow* w, int button, int action, int) { g_states[w].input.mouse.on_button(button, to_action(action)); });
	glfwSetCursorPosCallback(ret, [](GLFWwindow* w, double x, double y) { g_states[w].input.mouse.on_position(glm::tvec2<double>{x, y}); });
	glfwSetScrollCallback(ret, [](GLFWwindow* w, double x, double y) { g_states[w].input.mouse.on_scroll(glm::tvec2<double>{x, y}); });
	glfwSetDropCallback(ret, [](GLFWwindow* w, int count, char const** paths) {
		auto& file_drops = g_states[w].file_drops;
		for (int i = 0; i < count; ++i) { file_drops.push_back(paths[i]); }
	});
	return ret;
}

void Glfw::Deleter::operator()(Glfw const& glfw) const {
	if (!glfw.active) { return; }
	glfwTerminate();
}

void Glfw::Deleter::operator()(Window const& window) const {
	glfwDestroyWindow(window.win);
	g_states.erase(window.win);
}

std::vector<char const*> Glfw::vk_extensions() const {
	auto ret = std::vector<char const*>{};
	auto count = std::uint32_t{};
	auto* extensions = glfwGetRequiredInstanceExtensions(&count);
	for (std::uint32_t i = 0; i < count; ++i) { ret.push_back(extensions[i]); }
	return ret;
}

vk::UniqueSurfaceKHR Glfw::Window::make_surface(vk::Instance const instance) const {
	if (!instance) { return {}; }
	auto ret = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(instance, win, {}, &ret) != VK_SUCCESS) { throw InitError{"Failed to create Vulkan Surface"}; }
	return vk::UniqueSurfaceKHR{vk::SurfaceKHR{ret}, instance};
}

glm::uvec2 Glfw::Window::window_extent() const {
	auto ret = glm::ivec2{};
	glfwGetWindowSize(win, &ret.x, &ret.y);
	return ret;
}

glm::uvec2 Glfw::Window::framebuffer_extent() const {
	auto ret = glm::ivec2{};
	glfwGetFramebufferSize(win, &ret.x, &ret.y);
	return ret;
}

Glfw::State const& Glfw::Window::state() const {
	auto lock = std::scoped_lock{g_mutex};
	return g_states[win];
}
} // namespace facade
