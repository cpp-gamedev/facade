#include <facade/glfw/glfw_wsi.hpp>
#include <facade/render/renderer.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/util/time.hpp>
#include <facade/vk/pipes.hpp>
#include <facade/vk/render_frame.hpp>
#include <facade/vk/render_pass.hpp>
#include <facade/vk/swapchain.hpp>

namespace facade {
namespace {
vk::Format depth_format(vk::PhysicalDevice const gpu) {
	static constexpr auto target{vk::Format::eD32Sfloat};
	auto const props = gpu.getFormatProperties(target);
	if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) { return target; }
	return vk::Format::eD16Unorm;
}

constexpr auto get_samples(vk::SampleCountFlags supported, std::uint8_t desired) {
	if (desired >= 32 && (supported & vk::SampleCountFlagBits::e32)) { return vk::SampleCountFlagBits::e32; }
	if (desired >= 16 && (supported & vk::SampleCountFlagBits::e16)) { return vk::SampleCountFlagBits::e16; }
	if (desired >= 8 && (supported & vk::SampleCountFlagBits::e8)) { return vk::SampleCountFlagBits::e8; }
	if (desired >= 4 && (supported & vk::SampleCountFlagBits::e4)) { return vk::SampleCountFlagBits::e4; }
	if (desired >= 2 && (supported & vk::SampleCountFlagBits::e2)) { return vk::SampleCountFlagBits::e2; }
	return vk::SampleCountFlagBits::e1;
}

///
/// \brief Tracks frames per second
///
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

struct Renderer::Impl {
	Gfx gfx;
	vk::SampleCountFlags supported_msaa{};
	vk::SampleCountFlagBits msaa{};
	Glfw::Window window;
	Swapchain swapchain;

	Pipes pipes;
	RenderPass render_pass;
	RenderFrames<> render_frames;
	Gui* gui{};

	Shader::Db shader_db{};
	std::optional<RenderTarget> render_target{};
	Framebuffer framebuffer{};

	struct {
		std::optional<vk::PresentModeKHR> mode{};
		std::optional<ColourSpace> colour_space{};

		explicit operator bool() const { return mode || colour_space; }

		void reset() {
			mode.reset();
			colour_space.reset();
		}
	} requests{};

	Fps fps{};

	struct {
		FrameStats stats{};
		std::string gpu_name{};
		std::uint64_t triangles{};	// reset every frame
		std::uint32_t draw_calls{}; // reset every frame
	} stats{};

	Impl(Gfx gfx, Glfw::Window window, Gui* gui, Renderer::CreateInfo const& info)
		: gfx{gfx}, supported_msaa(gfx.gpu.getProperties().limits.framebufferColorSampleCounts),
		  msaa(get_samples(supported_msaa, info.desired_msaa)), window{window}, swapchain{gfx, GlfwWsi{window}.make_surface(gfx.instance)}, pipes(gfx, msaa),
		  render_pass(gfx, msaa, this->swapchain.info.imageFormat, depth_format(gfx.gpu)), render_frames(make_render_frames(gfx, info.command_buffers)),
		  gui(gui) {}

	void next_frame() {
		stats.stats.mode = swapchain.info.presentMode;
		stats.stats.fps = fps.next_frame();
		++stats.stats.frame_counter;
		stats.stats.triangles = std::exchange(stats.triangles, 0);
		stats.stats.draw_calls = std::exchange(stats.draw_calls, 0);
	}
};

Renderer::Renderer(Gfx gfx, Glfw::Window window, Gui* gui, CreateInfo const& info) : m_impl{std::make_unique<Impl>(std::move(gfx), window, gui, info)} {
	m_impl->swapchain.refresh(Swapchain::Spec{window.framebuffer_extent()});
	m_impl->stats.gpu_name = m_impl->gfx.gpu.getProperties().deviceName.data();
	m_impl->stats.stats.gpu_name = m_impl->stats.gpu_name;
	m_impl->stats.stats.msaa = m_impl->msaa;
	if (m_impl->gui) {
		m_impl->gui->init(Gui::InitInfo{
			.gfx = gfx,
			.window = window,
			.render_pass = m_impl->render_pass.render_pass(),
			.msaa = m_impl->msaa,
			.colour_space = m_impl->swapchain.colour_space(),
		});
	}
	logger::info("[Renderer] buffering (frames): [{}] | MSAA: [{}x] | max threads: [{}] |", buffering_v, to_int(m_impl->msaa), info.command_buffers);
}

Renderer::Renderer(Renderer&&) noexcept = default;
Renderer& Renderer::operator=(Renderer&&) noexcept = default;
Renderer::~Renderer() noexcept = default;

auto Renderer::info() const -> Info {
	return {
		.supported_msaa = m_impl->supported_msaa,
		.colour_space = m_impl->swapchain.colour_space(),
		.cbs_per_frame = m_impl->render_frames.get().secondary.size(),
	};
}

glm::uvec2 Renderer::framebuffer_extent() const { return m_impl->window.framebuffer_extent(); }

FrameStats const& Renderer::frame_stats() const { return m_impl->stats.stats; }

bool Renderer::is_supported(vk::PresentModeKHR const mode) const { return m_impl->swapchain.supported_present_modes().contains(mode); }

bool Renderer::request_mode(vk::PresentModeKHR const desired) {
	if (!is_supported(desired)) {
		logger::error("Unsupported present mode requested: [{}]", present_mode_str(desired));
		return false;
	}
	// queue up request to refresh swapchain at beginning of next frame
	if (m_impl->swapchain.info.presentMode != desired) { m_impl->requests.mode = desired; }
	return true;
}

void Renderer::request_colour_space(ColourSpace desired) {
	if (m_impl->swapchain.colour_space() == desired) { return; }
	// queue up request to refresh swapchain at beginning of next frame
	m_impl->requests.colour_space = desired;
}

bool Renderer::next_frame(std::span<vk::CommandBuffer> out) {
	assert(out.size() <= m_impl->render_frames.get().secondary.size());
	m_impl->next_frame();

	auto& frame = m_impl->render_frames.get();

	// "return" wrapper
	auto fill_and_return = [out, &frame] {
		auto span = frame.secondary.span();
		for (std::size_t i = 0; i < out.size(); ++i) { out[i] = span[i]; }
		return true;
	};

	// already have an acquired image
	if (m_impl->render_target) { return fill_and_return(); }

	if (m_impl->gui) {
		// ImGui NewFrame / EndFrame are called even if acquire / present fails
		// this allows user code to unconditionally call ImGui:: code in a frame, regardless of whether it will be drawn / presented
		// m_impl->gui->new_frame();
	}

	// check for pending swapchain refresh requests
	if (m_impl->requests) {
		auto const spec = [&] {
			auto ret = Swapchain::Spec{};
			if (m_impl->requests.colour_space) { ret.colour_space = *m_impl->requests.colour_space; }
			if (m_impl->requests.mode) { ret.mode = *m_impl->requests.mode; }
			m_impl->requests.reset();
			return ret;
		}();
		m_impl->swapchain.refresh(spec);
	}

	// acquire swapchain image
	auto acquired = ImageView{};
	if (m_impl->swapchain.acquire(m_impl->window.framebuffer_extent(), acquired, *frame.sync.draw) != vk::Result::eSuccess) { return false; }
	m_impl->gfx.reset(*frame.sync.drawn);
	m_impl->gfx.shared->defer_queue.next();

	// refresh render target
	m_impl->render_target = m_impl->render_pass.refresh(acquired);

	// refresh framebuffer
	m_impl->framebuffer = frame.refresh(m_impl->render_pass.render_pass(), *m_impl->render_target);

	// begin recording commands
	auto const cbii = vk::CommandBufferInheritanceInfo{m_impl->render_pass.render_pass(), 0, m_impl->framebuffer.framebuffer};
	for (auto const cb : m_impl->framebuffer.secondary) { cb.begin({vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii}); }
	return fill_and_return();
}

Pipeline Renderer::bind_pipeline(vk::CommandBuffer cb, Pipeline::State const& state, std::string const& shader_id) {
	// obtain pipeline and bind it
	auto const shader = m_impl->shader_db.find(shader_id);
	if (!shader) { throw Error{fmt::format("Failed to find shader: {}", shader_id)}; }
	auto ret = m_impl->pipes.get(m_impl->render_pass.render_pass(), state, shader);
	ret.bind(cb);

	// set viewport and scissor
	glm::vec2 const extent = glm::uvec2{m_impl->render_target->extent.width, m_impl->render_target->extent.height};
	auto viewport = vk::Viewport{0.0f, extent.y, extent.x, -extent.y, 0.0f, 1.0f};
	auto scissor = vk::Rect2D{{}, m_impl->render_target->extent};
	cb.setViewport(0U, viewport);
	cb.setScissor(0U, scissor);
	return ret;
}

bool Renderer::render() {
	if (m_impl->gui) {
		// ImGui NewFrame / EndFrame are called even if acquire / present fails
		// this allows user code to unconditionally call ImGui:: code in a frame, regardless of whether it will be drawn / presented
		// m_impl->gui->end_frame();
	}
	if (!m_impl->render_target) { return false; }

	auto& frame = m_impl->render_frames.get();

	auto const cbs = frame.secondary.span();
	// perform the actual Dear ImGui render
	if (m_impl->gui) { m_impl->gui->render(cbs.front()); }
	// end recording
	for (auto cb : cbs) { cb.end(); }

	// prepare render pass
	frame.primary.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	// transition render target to be ready to draw to
	m_impl->render_target->to_draw(frame.primary);

	auto const clear_colour = vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}};
	// execute render pass
	m_impl->render_pass.execute(m_impl->framebuffer, clear_colour);

	// transition render target to be ready to be presented
	m_impl->render_target->to_present(frame.primary);
	// end render pass
	frame.primary.end();

	// submit commands
	{
		auto lock = std::scoped_lock{m_impl->gfx.shared->mutex};
		frame.submit(m_impl->gfx.queue);
	}
	// present image
	m_impl->swapchain.present(m_impl->window.framebuffer_extent(), *frame.sync.present);

	// rotate everything
	m_impl->pipes.rotate();
	m_impl->render_frames.rotate();

	// clear render target
	m_impl->render_target.reset();

	// update stats
	++m_impl->stats.stats.frame_counter;

	return true;
}

void Renderer::draw(Pipeline& pipeline, StaticMesh const& mesh, std::span<glm::mat4x4 const> instances) {
	// update stats
	++m_impl->stats.draw_calls;
	m_impl->stats.triangles += instances.size() * mesh.view().vertex_count / 3;

	// draw
	pipeline.draw(mesh, instances);
}

Shader Renderer::add_shader(std::string id, SpirV vert, SpirV frag) { return m_impl->shader_db.add(std::move(id), std::move(vert), std::move(frag)); }
bool Renderer::add_shader(Shader shader) { return m_impl->shader_db.add(std::move(shader)); }
Shader Renderer::find_shader(std::string const& id) const { return m_impl->shader_db.find(id); }
} // namespace facade
