#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <facade/engine/engine.hpp>
#include <facade/glfw/glfw_wsi.hpp>
#include <facade/render/renderer.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/vk.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>

namespace facade {
namespace {
static constexpr std::size_t command_buffers_v{1};

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
	std::uint8_t msaa;

	RenderWindow(glm::uvec2 extent, char const* title, std::unique_ptr<Gui> gui, std::uint8_t msaa)
		: window(make_window(extent, title)), vulkan(GlfwWsi{window}), gfx{vulkan.gfx()},
		  renderer(gfx, window, std::move(gui), Renderer::CreateInfo{command_buffers_v, msaa}), msaa(msaa) {}
};
} // namespace

struct Engine::Impl {
	RenderWindow window;

	Impl(CreateInfo const& info) : window(info.extent, info.title, std::make_unique<DearImGui>(), info.msaa_samples) {}
};

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;
Engine::~Engine() noexcept = default;

Engine::Engine(CreateInfo const& info) : m_impl(std::make_unique<Impl>(info)) {
	if (info.auto_show) { show_window(); }
}

bool Engine::add_shader(Shader shader) { return m_impl->window.renderer.add_shader(shader); }

void Engine::show_window() { glfwShowWindow(m_impl->window.window.get()); }

void Engine::hide_window() { glfwHideWindow(m_impl->window.window.get()); }

bool Engine::running() const { return !glfwWindowShouldClose(m_impl->window.window.get()); }

bool Engine::next_frame(vk::CommandBuffer& out) {
	m_impl->window.window.get().glfw->poll_events();
	if (!m_impl->window.renderer.next_frame({&out, 1})) { return false; }
	return true;
}

void Engine::submit() { m_impl->window.renderer.render(); }

void Engine::request_stop() { glfwSetWindowShouldClose(m_impl->window.window.get(), GLFW_TRUE); }

Gfx const& Engine::gfx() const { return m_impl->window.gfx; }
Glfw::Window Engine::window() const { return m_impl->window.window; }
Input const& Engine::input() const { return m_impl->window.window.get().state().input; }
Renderer& Engine::renderer() const { return m_impl->window.renderer; }
} // namespace facade
