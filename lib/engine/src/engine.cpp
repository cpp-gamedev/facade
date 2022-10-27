#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <facade/defines.hpp>
#include <facade/engine/engine.hpp>
#include <facade/glfw/glfw_wsi.hpp>
#include <facade/render/renderer.hpp>
#include <facade/scene/scene_renderer.hpp>
#include <facade/util/error.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/vk.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>

namespace facade {
namespace {
static constexpr std::size_t command_buffers_v{1};

bool determine_validation(Validation const desired) {
	switch (desired) {
	case Validation::eForceOff: return false;
	case Validation::eForceOn: return true;
	default: break;
	}

	// TODO: also enable if debugger detected
	return debug_v;
}

UniqueWin make_window(glm::ivec2 extent, char const* title) {
	auto ret = Glfw::Window::make();
	glfwSetWindowTitle(ret.get(), title);
	glfwSetWindowSize(ret.get(), extent.x, extent.y);
	return ret;
}

vk::UniqueDescriptorPool make_pool(vk::Device const device) {
	vk::DescriptorPoolSize pool_sizes[] = {
		{vk::DescriptorType::eSampledImage, 1000},		   {vk::DescriptorType::eCombinedImageSampler, 1000}, {vk::DescriptorType::eSampledImage, 1000},
		{vk::DescriptorType::eStorageImage, 1000},		   {vk::DescriptorType::eUniformTexelBuffer, 1000},	  {vk::DescriptorType::eStorageTexelBuffer, 1000},
		{vk::DescriptorType::eUniformBuffer, 1000},		   {vk::DescriptorType::eStorageBuffer, 1000},		  {vk::DescriptorType::eUniformBufferDynamic, 1000},
		{vk::DescriptorType::eStorageBufferDynamic, 1000}, {vk::DescriptorType::eInputAttachment, 1000},
	};
	auto pool_info = vk::DescriptorPoolCreateInfo{};
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000 * std::size(pool_sizes);
	pool_info.poolSizeCount = static_cast<std::uint32_t>(std::size(pool_sizes));
	pool_info.pPoolSizes = pool_sizes;
	return device.createDescriptorPoolUnique(pool_info);
}

glm::vec4 to_linear(glm::vec4 const& srgb) { return glm::convertSRGBToLinear(srgb); }

void correct_imgui_style() {
	auto* colours = ImGui::GetStyle().Colors;
	for (int i = 0; i < ImGuiCol_COUNT; ++i) {
		auto& colour = colours[i];
		auto const corrected = to_linear(glm::vec4{colour.x, colour.y, colour.z, colour.w});
		colour = {corrected.x, corrected.y, corrected.z, corrected.w};
	}
}

vk::UniqueDescriptorPool init_imgui(Gui::InitInfo const& create_info) {
	auto pool = make_pool(create_info.gfx.device);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	if (create_info.colour_space == ColourSpace::eSrgb) { correct_imgui_style(); }

	auto loader = vk::DynamicLoader{};
	auto get_fn = [&loader](char const* name) { return loader.getProcAddress<PFN_vkVoidFunction>(name); };
	auto lambda = +[](char const* name, void* ud) {
		auto const* gf = reinterpret_cast<decltype(get_fn)*>(ud);
		return (*gf)(name);
	};
	ImGui_ImplVulkan_LoadFunctions(lambda, &get_fn);
	ImGui_ImplGlfw_InitForVulkan(create_info.window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = create_info.gfx.instance;
	init_info.PhysicalDevice = create_info.gfx.gpu;
	init_info.Device = create_info.gfx.device;
	init_info.QueueFamily = create_info.gfx.queue_family;
	init_info.Queue = create_info.gfx.queue;
	init_info.DescriptorPool = *pool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(create_info.msaa);

	ImGui_ImplVulkan_Init(&init_info, create_info.render_pass);

	{
		auto cmd = Cmd{create_info.gfx};
		ImGui_ImplVulkan_CreateFontsTexture(cmd.cb);
	}
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	return pool;
}

struct DearImGui : Gui {
	vk::UniqueDescriptorPool pool{};

	~DearImGui() override {
		if (!pool) { return; }
		pool.getOwner().waitIdle();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void init(InitInfo const& info) final {
		assert(!pool);
		pool = init_imgui(info);
	}

	void new_frame() final {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void end_frame() final {
		// ImGui::Render calls ImGui::EndFrame
		ImGui::Render();
	}

	void render(vk::CommandBuffer cb) final { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb); }
};

struct RenderWindow {
	UniqueWin window;
	Vulkan vulkan;
	Gfx gfx;
	Renderer renderer;
	std::unique_ptr<Gui> gui;

	RenderWindow(UniqueWin window, std::unique_ptr<Gui> gui, std::uint8_t msaa, bool validation)
		: window(std::move(window)), vulkan(GlfwWsi{this->window}, validation), gfx(vulkan.gfx()),
		  renderer(gfx, this->window, gui.get(), Renderer::CreateInfo{command_buffers_v, msaa}), gui(std::move(gui)) {}
};
} // namespace

struct Engine::Impl {
	RenderWindow window;
	SceneRenderer renderer;
	Scene scene;

	std::uint8_t msaa;
	DeltaTime dt{};
	vk::CommandBuffer cb{};

	Impl(UniqueWin window, std::uint8_t msaa, bool validation)
		: window(std::move(window), std::make_unique<DearImGui>(), msaa, validation), renderer(this->window.gfx), scene(this->window.gfx), msaa(msaa) {
		s_instance = this;
	}

	~Impl() {
		window.gfx.device.waitIdle();
		s_instance = {};
	}

	Impl& operator=(Impl&&) = delete;
};

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;
Engine::~Engine() noexcept = default;

bool Engine::is_instance_active() { return s_instance != nullptr; }

Engine::Engine(CreateInfo const& info) noexcept(false) {
	if (s_instance) { throw Error{"Engine: active instance exists and has not been destroyed"}; }
	m_impl = std::make_unique<Impl>(make_window(info.extent, info.title), info.desired_msaa, determine_validation(info.validation));
	if (info.auto_show) { show(true); }
}

void Engine::add_shader(Shader shader) { m_impl->window.renderer.add_shader(std::move(shader)); }

void Engine::show(bool reset_dt) {
	glfwShowWindow(window());
	if (reset_dt) { m_impl->dt = {}; }
}

void Engine::hide() { glfwHideWindow(window()); }

bool Engine::running() const { return !glfwWindowShouldClose(window()); }

float Engine::next_frame() {
	window().glfw->poll_events();
	m_impl->window.gui->new_frame();
	if (!m_impl->window.renderer.next_frame({&m_impl->cb, 1})) { m_impl->cb = vk::CommandBuffer{}; }
	return m_impl->dt();
}

void Engine::render() {
	if (m_impl->cb) { m_impl->renderer.render(scene(), renderer(), m_impl->cb); }
	m_impl->window.gui->end_frame();
	m_impl->window.renderer.render();
}

void Engine::request_stop() { glfwSetWindowShouldClose(window(), GLFW_TRUE); }

Scene& Engine::scene() const { return m_impl->scene; }
Gfx const& Engine::gfx() const { return m_impl->window.gfx; }
Glfw::Window const& Engine::window() const { return m_impl->window.window; }
Glfw::State const& Engine::state() const { return window().state(); }
Input const& Engine::input() const { return state().input; }
Renderer& Engine::renderer() const { return m_impl->window.renderer; }
} // namespace facade
