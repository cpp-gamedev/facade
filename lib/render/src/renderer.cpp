#include <facade/glfw/glfw_wsi.hpp>
#include <facade/render/renderer.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/util/time.hpp>
#include <facade/vk/pipes.hpp>
#include <facade/vk/render_frame.hpp>
#include <facade/vk/render_pass.hpp>
#include <facade/vk/static_mesh.hpp>
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

	std::string gpu_name{};

	Impl(Gfx gfx, Glfw::Window window, Gui* gui, Renderer::CreateInfo const& info)
		: gfx{gfx}, supported_msaa(gfx.gpu.getProperties().limits.framebufferColorSampleCounts),
		  msaa(get_samples(supported_msaa, info.desired_msaa)), window{window}, swapchain{gfx, GlfwWsi{window}.make_surface(gfx.instance)}, pipes(gfx, msaa),
		  render_pass(gfx, msaa, this->swapchain.info.imageFormat, depth_format(gfx.gpu)), render_frames(make_render_frames(gfx, info.command_buffers)),
		  gui(gui) {}
};

Renderer::Renderer(Gfx const& gfx, Glfw::Window window, Gui* gui, CreateInfo const& create_info)
	: m_impl{std::make_unique<Impl>(std::move(gfx), window, gui, create_info)} {
	m_impl->swapchain.refresh(Swapchain::Spec{window.framebuffer_extent()});
	m_impl->gpu_name = m_impl->gfx.gpu.getProperties().deviceName.data();
	if (m_impl->gui) {
		m_impl->gui->init(Gui::InitInfo{
			.gfx = gfx,
			.window = window,
			.render_pass = m_impl->render_pass.render_pass(),
			.msaa = m_impl->msaa,
			.colour_space = m_impl->swapchain.colour_space(),
		});
	}
	logger::info("[Renderer] buffering (frames): [{}] | MSAA: [{}x] | max threads: [{}] |", buffering_v, to_int(m_impl->msaa), create_info.command_buffers);
}

Renderer::Renderer(Renderer&&) noexcept = default;
Renderer& Renderer::operator=(Renderer&&) noexcept = default;
Renderer::~Renderer() noexcept = default;

auto Renderer::info() const -> Info {
	return {
		.present_mode = m_impl->swapchain.info.presentMode,
		.supported_msaa = m_impl->supported_msaa,
		.current_msaa = m_impl->msaa,
		.colour_space = m_impl->swapchain.colour_space(),
		.cbs_per_frame = m_impl->render_frames.get().secondary.size(),
	};
}

std::string_view Renderer::gpu_name() const { return m_impl->gpu_name; }

glm::uvec2 Renderer::framebuffer_extent() const { return m_impl->window.framebuffer_extent(); }

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

	auto& frame = m_impl->render_frames.get();

	// "return" wrapper
	auto fill_and_return = [out, &frame] {
		auto span = frame.secondary.span();
		for (std::size_t i = 0; i < out.size(); ++i) { out[i] = span[i]; }
		return true;
	};

	// already have an acquired image
	if (m_impl->render_target) { return fill_and_return(); }

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
	{
		auto lock = std::scoped_lock{m_impl->gfx.shared->mutex};
		if (m_impl->swapchain.acquire(lock, m_impl->window.framebuffer_extent(), acquired, *frame.sync.draw) != vk::Result::eSuccess) { return false; }
	}
	m_impl->gfx.reset(*frame.sync.drawn);
	m_impl->gfx.shared->defer_queue.next();

	// refresh render target
	m_impl->render_target = m_impl->render_pass.refresh(acquired);

	// refresh framebuffer
	m_impl->framebuffer = frame.refresh(m_impl->gfx.device, m_impl->render_pass.render_pass(), *m_impl->render_target);

	// begin recording commands
	auto const cbii = vk::CommandBufferInheritanceInfo{m_impl->render_pass.render_pass(), 0, m_impl->framebuffer.framebuffer};
	for (auto const cb : m_impl->framebuffer.secondary) { cb.begin({vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii}); }
	return fill_and_return();
}

Pipeline Renderer::bind_pipeline(vk::CommandBuffer cb, VertexInput const& vlayout, Pipeline::State state, RenderShader shader) {
	// obtain pipeline and bind it
	auto const vert = m_impl->shader_db.find(shader.vert);
	auto const frag = m_impl->shader_db.find(shader.frag);
	if (!vert) { throw Error{fmt::format("Failed to find vertex shader: {}", shader.vert)}; }
	if (!vert) { throw Error{fmt::format("Failed to find fragment shader: {}", shader.frag)}; }
	if (vlayout.attributes.empty() || vlayout.bindings.empty()) { throw Error{fmt::format("Invalid vertex layout")}; }
	auto ret = m_impl->pipes.get(m_impl->render_pass.render_pass(), state, vlayout, {vert, frag});
	ret.bind(cb);

	// set viewport and scissor
	glm::vec2 const extent = glm::uvec2{m_impl->render_target->extent.width, m_impl->render_target->extent.height};
	auto viewport = vk::Viewport{0.0f, extent.y, extent.x, -extent.y, 0.0f, 1.0f};
	auto scissor = vk::Rect2D{{}, m_impl->render_target->extent};
	cb.setViewport(0u, viewport);
	cb.setScissor(0u, scissor);
	return ret;
}

Pipeline Renderer::bind_pipeline(vk::CommandBuffer cb, VertexLayout const& vlayout, Pipeline::State state, Shader::Id id_frag) {
	auto const vert = m_impl->shader_db.find(vlayout.shader);
	auto const frag = m_impl->shader_db.find(id_frag);
	if (!vert) { throw Error{fmt::format("Failed to find vertex shader: {}", vlayout.shader)}; }
	if (!vert) { throw Error{fmt::format("Failed to find fragment shader: {}", id_frag)}; }
	if (vlayout.input.attributes.empty() || vlayout.input.bindings.empty()) { throw Error{fmt::format("Invalid vertex input")}; }
	auto ret = m_impl->pipes.get(m_impl->render_pass.render_pass(), state, vlayout.input, {vert, frag});
	ret.bind(cb);

	// set viewport and scissor
	glm::vec2 const extent = glm::uvec2{m_impl->render_target->extent.width, m_impl->render_target->extent.height};
	auto viewport = vk::Viewport{0.0f, extent.y, extent.x, -extent.y, 0.0f, 1.0f};
	auto scissor = vk::Rect2D{{}, m_impl->render_target->extent};
	cb.setViewport(0u, viewport);
	cb.setScissor(0u, scissor);
	return ret;
}

bool Renderer::render() {
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
		frame.submit(lock, m_impl->gfx.queue);

		// present image
		m_impl->swapchain.present(lock, m_impl->window.framebuffer_extent(), *frame.sync.present);
	}

	// rotate everything
	m_impl->pipes.rotate();
	m_impl->render_frames.rotate();

	// clear render target
	m_impl->render_target.reset();

	return true;
}

Shader Renderer::add_shader(std::string id, SpirV spir_v) { return m_impl->shader_db.add(std::move(id), std::move(spir_v)); }
bool Renderer::add_shader(Shader shader) { return m_impl->shader_db.add(std::move(shader)); }
Shader Renderer::find_shader(std::string const& id) const { return m_impl->shader_db.find(id); }

Gfx const& Renderer::gfx() const { return m_impl->gfx; }
} // namespace facade
