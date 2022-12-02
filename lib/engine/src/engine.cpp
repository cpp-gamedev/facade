#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <djson/json.hpp>
#include <facade/defines.hpp>
#include <facade/engine/engine.hpp>
#include <facade/engine/scene_renderer.hpp>
#include <facade/glfw/glfw_wsi.hpp>
#include <facade/render/renderer.hpp>
#include <facade/scene/gltf_loader.hpp>
#include <facade/util/data_provider.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/env.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/util/thread_pool.hpp>
#include <facade/util/zip_ranges.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/skybox.hpp>
#include <facade/vk/vk.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>
#include <filesystem>

namespace facade {
namespace fs = std::filesystem;

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
	enum class State { eNewFrame, eEndFrame };

	vk::UniqueDescriptorPool pool{};
	State state{};

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

	void new_frame() {
		if (state == State::eEndFrame) { end_frame(); }
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		state = State::eEndFrame;
	}

	void end_frame() {
		// ImGui::Render calls ImGui::EndFrame
		ImGui::Render();
		state = State::eNewFrame;
	}

	void render(vk::CommandBuffer cb) final { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb); }
};

struct RenderWindow {
	UniqueWin window;
	Vulkan vulkan;
	Gfx gfx;
	Renderer renderer;
	std::unique_ptr<DearImGui> gui;

	RenderWindow(UniqueWin window, std::unique_ptr<DearImGui> gui, std::uint8_t msaa, bool validation)
		: window(std::move(window)), vulkan(GlfwWsi{this->window}, validation), gfx(vulkan.gfx()),
		  renderer(gfx, this->window, gui.get(), Renderer::CreateInfo{command_buffers_v, msaa}), gui(std::move(gui)) {}
};

bool load_gltf(Scene& out_scene, char const* path, AtomicLoadStatus& out_status, ThreadPool* thread_pool) {
	auto const provider = FileDataProvider::mount_parent_dir(path);
	auto json = dj::Json::from_file(path);
	return Scene::GltfLoader{out_scene, out_status}(json, provider, thread_pool);
}

std::optional<Skybox::Data> load_skybox_data(char const* path, AtomicLoadStatus& out_status, ThreadPool* thread_pool) {
	LoadFuture<Image> images[6]{};
	static constexpr std::string_view faces_v[] = {"x+", "x-", "y+", "y-", "z+", "z-"};
	out_status.total = 6;
	out_status.done = 0;
	out_status.stage = LoadStage::eUploadingResources;

	auto const provider = FileDataProvider::mount_parent_dir(path);
	auto load_image = [&provider, &out_status, thread_pool](std::string_view uri) {
		auto load = [&provider, uri] { return Image{provider.load(uri).span(), std::string{uri}}; };
		if (thread_pool) {
			return LoadFuture<Image>{*thread_pool, out_status.done, load};
		} else {
			return LoadFuture<Image>{out_status.done, load};
		}
	};
	auto json = dj::Json::from_file(path);
	if (!json) {
		// TODO: error
		return {};
	}

	for (auto [face, image] : zip_ranges(faces_v, images)) { image = load_image(json[face].as_string()); }

	if (!std::all_of(std::begin(images), std::end(images), [](MaybeFuture<Image> const& i) { return i.active(); })) {
		// TODO: error
		return {};
	}

	auto ret = Skybox::Data{};
	for (auto [in, out] : zip_ranges(images, ret.images)) { out = in.get(); }
	return ret;
}

template <typename T>
bool ready(std::future<T> const& future) {
	return future.valid() && future.wait_for(std::chrono::seconds{}) == std::future_status::ready;
}

template <typename T>
bool timeout(std::future<T> const& future) {
	return future.valid() && future.wait_for(std::chrono::seconds{}) == std::future_status::timeout;
}

template <typename T>
bool busy(std::future<T> const& future) {
	return future.valid() && future.wait_for(std::chrono::seconds{}) == std::future_status::deferred;
}

struct LoadRequest {
	std::string path{};
	std::future<Scene> scene{};
	std::future<std::optional<Skybox::Data>> skybox_data{};
	AtomicLoadStatus status{};
	float start_time{};
};

struct Fps {
	std::uint32_t frames{};
	std::uint32_t fps{};
	float start{time::since_start()};

	std::uint32_t next_frame() {
		++frames;
		if (auto const now = time::since_start(); now - start >= 1.0f) {
			start = now;
			fps = std::exchange(frames, 0);
		}
		return fps;
	}
};
} // namespace

struct Engine::Impl {
	RenderWindow window;
	SceneRenderer renderer;
	Scene scene;
	Skybox skybox;

	std::uint8_t msaa;

	ThreadPool thread_pool{};
	std::mutex mutex{};
	Stats stats{};
	Fps fps{};

	struct {
		LoadRequest request{};
		UniqueTask<void()> callback{};

		bool active() const { return request.scene.valid() || request.skybox_data.valid(); }
	} load{};

	Impl(UniqueWin window, std::uint8_t msaa, bool validation, std::optional<std::uint32_t> thread_count)
		: window(std::move(window), std::make_unique<DearImGui>(), msaa, validation), renderer(this->window.renderer), scene(this->window.gfx),
		  skybox(this->window.gfx), msaa(msaa), thread_pool(thread_count) {
		s_instance = this;
		load.request.status.reset();
	}

	~Impl() {
		load.request.scene = {};
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
	if (info.force_thread_count) { logger::info("[Engine] Forcing load thread count: [{}]", *info.force_thread_count); }
	m_impl = std::make_unique<Impl>(make_window(info.extent, info.title), info.desired_msaa, determine_validation(info.validation), info.force_thread_count);
	if (info.auto_show) { show(true); }
}

void Engine::add_shader(Shader shader) { m_impl->window.renderer.add_shader(std::move(shader)); }

void Engine::show(bool reset_dt) {
	glfwShowWindow(window());
	if (reset_dt) { m_impl->window.window.get().glfw->reset_dt(); }
}

void Engine::hide() { glfwHideWindow(window()); }

bool Engine::running() const { return !glfwWindowShouldClose(window()); }

auto Engine::poll() -> State const& {
	// the code in this call locks the mutex, so it's not inlined here
	update_load_request();
	// ImGui wants all widget calls within BeginFrame() / EndFrame(), so begin here
	m_impl->window.gui->new_frame();
	m_impl->window.window.get().glfw->poll_events();
	auto const& ret = m_impl.get()->window.window.get().state();
	m_impl->scene.tick(ret.dt);
	m_impl->stats.fps = m_impl->fps.next_frame();
	++m_impl->stats.frame_counter;
	return ret;
}

void Engine::render() {
	auto cb = vk::CommandBuffer{};
	// we skip rendering the scene if acquiring a swapchain image fails (unlikely)
	if (m_impl->window.renderer.next_frame({&cb, 1})) { m_impl->renderer.render(scene(), &m_impl->skybox, renderer(), cb); }
	m_impl->window.gui->end_frame();
	m_impl->window.renderer.render();
	{
		auto const& info = m_impl->renderer.info();
		m_impl->stats.draw_calls = info.draw_calls;
		m_impl->stats.triangles = info.triangles_drawn;
	}
	{
		auto info = m_impl->window.renderer.info();
		m_impl->stats.mode = info.present_mode;
		m_impl->stats.current_msaa = info.current_msaa;
		m_impl->stats.supported_msaa = info.supported_msaa;
	}
}

void Engine::request_stop() { glfwSetWindowShouldClose(window(), GLFW_TRUE); }

glm::ivec2 Engine::window_position() const { return m_impl->window.window.get().position(); }
glm::uvec2 Engine::window_extent() const { return m_impl->window.window.get().window_extent(); }
glm::uvec2 Engine::framebuffer_extent() const { return m_impl->window.window.get().framebuffer_extent(); }

Stats const& Engine::stats() const { return m_impl->stats; }
std::string_view Engine::gpu_name() const { return m_impl->window.renderer.gpu_name(); }

bool Engine::load_async(std::string json_path, UniqueTask<void()> on_loaded) {
	if (!fs::is_regular_file(json_path)) {
		// early return if file will fail to load anyway
		logger::error("[Engine] Invalid GLTF JSON path: [{}]", json_path);
		return false;
	}

	// ensure thread pool queue has at least one worker thread, else load on this thread
	if (m_impl->thread_pool.thread_count() == 0) {
		auto const start = time::since_start();
		if (load_gltf(m_impl->scene, json_path.c_str(), m_impl->load.request.status, nullptr)) {
			logger::info("...GLTF [{}] loaded in [{:.2f}s]", env::to_filename(json_path), time::since_start() - start);
			return true;
		}
		if (auto sd = load_skybox_data(json_path.c_str(), m_impl->load.request.status, nullptr)) {
			m_impl->skybox.set(sd->views());
			logger::info("...Skybox [{}] loaded in [{:.2f}s]", env::to_filename(json_path), time::since_start() - start);
		}
		logger::error("[Engine] Failed to load file: [{}]", json_path);
		return false;
	}

	// shared state will need to be accessed, lock the mutex
	auto lock = std::scoped_lock{m_impl->mutex};
	if (m_impl->load.active()) {
		// we don't support discarding in-flight requests
		logger::warn("[Engine] Denied attempt to load_async when a load request is already in flight");
		return false;
	}

	// ready to start loading
	std::string_view const type = fs::path{json_path}.extension() == ".skybox" ? "Skybox" : "GLTF";
	logger::info("[Engine] Loading {} [{}]...", type, env::to_filename(json_path));
	// populate load request
	m_impl->load.callback = std::move(on_loaded);
	m_impl->load.request.path = std::move(json_path);
	m_impl->load.request.status.reset();
	m_impl->load.request.start_time = time::since_start();
	// if thread pool queue has only one worker thread, can't dispatch tasks from within a task and then wait for them (deadlock)
	auto* tp = m_impl->thread_pool.thread_count() > 1 ? &m_impl->thread_pool : nullptr;
	if (type == "Skybox") {
		auto func = [path = m_impl->load.request.path, status = &m_impl->load.request.status, tp] { return load_skybox_data(path.c_str(), *status, tp); };
		// store future
		m_impl->load.request.skybox_data = m_impl->thread_pool.enqueue(func);
	} else {
		auto func = [path = m_impl->load.request.path, gfx = m_impl->window.gfx, status = &m_impl->load.request.status, tp] {
			auto scene = Scene{gfx};
			if (!load_gltf(scene, path.c_str(), *status, tp)) { logger::error("[Engine] Failed to load GLTF: [{}]", path); }
			// return the scene even on failure, it will be empty but valid
			return scene;
		};
		// store future
		m_impl->load.request.scene = m_impl->thread_pool.enqueue(func);
	}
	return true;
}

LoadStatus Engine::load_status() const {
	auto lock = std::scoped_lock{m_impl->mutex};
	auto const& status = m_impl->load.request.status;
	return {.stage = status.stage.load(), .total = status.total, .done = status.done};
}

std::size_t Engine::load_thread_count() const { return m_impl->thread_pool.thread_count(); }

Scene& Engine::scene() const { return m_impl->scene; }
GLFWwindow* Engine::window() const { return m_impl->window.window.get(); }
Glfw::State const& Engine::state() const { return m_impl->window.window.get().state(); }
Input const& Engine::input() const { return state().input; }
Renderer& Engine::renderer() const { return m_impl->window.renderer; }

void Engine::update_load_request() {
	auto lock = std::unique_lock{m_impl->mutex};
	// early return if future isn't valid or is still busy
	if (ready(m_impl->load.request.scene)) {
		// transfer scene (under mutex lock)
		m_impl->scene = m_impl->load.request.scene.get();
		// reset load status
		m_impl->load.request.status.reset();
		// move out the path
		auto path = std::move(m_impl->load.request.path);
		// move out the callback
		auto callback = std::move(m_impl->load.callback);
		auto const duration = time::since_start() - m_impl->load.request.start_time;
		// unlock mutex to prevent possible deadlock (eg callback calls load_gltf again)
		lock.unlock();
		logger::info("...GLTF [{}] loaded in [{:.2f}s]", env::to_filename(path), duration);
		// invoke callback
		if (callback) { callback(); }
	}

	if (ready(m_impl->load.request.skybox_data)) {
		auto skybox_data = m_impl->load.request.skybox_data.get();
		m_impl->load.request.status.reset();
		auto path = std::move(m_impl->load.request.path);
		if (skybox_data) {
			m_impl->skybox.set(skybox_data->views());
			auto const duration = time::since_start() - m_impl->load.request.start_time;
			lock.unlock();
			auto callback = std::move(m_impl->load.callback);
			logger::info("...Skybox [{}] loaded in [{:.2f}s]", env::to_filename(path), duration);
			if (callback) { callback(); }
		} else {
			logger::warn("...Skybox [{}] failed to load", env::to_filename(path));
		}
	}
}
} // namespace facade
