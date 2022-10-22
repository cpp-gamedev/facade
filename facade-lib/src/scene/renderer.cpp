#include <facade/dear_imgui/dear_imgui.hpp>
#include <facade/scene/renderer.hpp>
#include <facade/util/error.hpp>
#include <facade/util/string.hpp>
#include <facade/vk/pipes.hpp>
#include <facade/vk/render_frame.hpp>
#include <facade/vk/render_pass.hpp>

namespace facade {
namespace {
vk::Format depth_format(vk::PhysicalDevice const gpu) {
	static constexpr auto target{vk::Format::eD32Sfloat};
	auto const props = gpu.getFormatProperties(target);
	if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) { return target; }
	return vk::Format::eD16Unorm;
}
} // namespace

struct Renderer::Impl {
	Gfx gfx;
	Glfw::Window window;
	Swapchain swapchain;

	Pipes pipes;
	RenderPass render_pass;
	RenderFrames<> render_frames;
	DearImgui dear_imgui;

	Shader::Db shader_db{};
	std::optional<RenderTarget> render_target{};
	Framebuffer framebuffer{};

	Impl(Gfx gfx, Glfw::Window window, Renderer::Info const& info)
		: gfx{gfx}, window{window}, swapchain{gfx, GlfwWsi{window}.make_surface(gfx.instance), vk::PresentModeKHR::eFifo}, pipes(gfx, info.samples),
		  render_pass(gfx, info.samples, this->swapchain.info.imageFormat, depth_format(gfx.gpu)), render_frames(make_render_frames(gfx, info.command_buffers)),
		  dear_imgui(DearImgui::Info{
			  gfx,
			  window,
			  render_pass.render_pass(),
			  info.samples,
		  }) {}
};

Renderer::Renderer(Gfx gfx, Glfw::Window window, Info const& info) : m_impl{std::make_unique<Impl>(std::move(gfx), window, info)} {
	m_impl->swapchain.refresh(Swapchain::Spec{window.framebuffer_extent()});
}

Renderer::Renderer(Renderer&&) noexcept = default;
Renderer& Renderer::operator=(Renderer&&) noexcept = default;
Renderer::~Renderer() noexcept = default;

glm::uvec2 Renderer::framebuffer_extent() const { return m_impl->window.framebuffer_extent(); }

std::size_t Renderer::command_buffers_per_frame() const { return m_impl->render_frames.get().secondary.span().size(); }

bool Renderer::next_frame(std::span<vk::CommandBuffer> out) {
	assert(out.size() <= command_buffers_per_frame());
	auto& frame = m_impl->render_frames.get();
	auto fill_and_return = [out, &frame] {
		auto span = frame.secondary.span();
		for (std::size_t i = 0; i < out.size(); ++i) { out[i] = span[i]; }
		return true;
	};
	if (m_impl->render_target) { return fill_and_return(); }

	auto acquired = ImageView{};
	if (m_impl->swapchain.acquire(m_impl->window.framebuffer_extent(), acquired, *frame.sync.draw) != vk::Result::eSuccess) { return false; }
	m_impl->gfx.reset(*frame.sync.drawn);
	m_impl->dear_imgui.new_frame();
	m_impl->gfx.shared->defer_queue.next();

	m_impl->render_target = m_impl->render_pass.refresh(acquired);

	m_impl->framebuffer = frame.refresh(m_impl->render_pass.render_pass(), *m_impl->render_target);
	auto const cbii = vk::CommandBufferInheritanceInfo{m_impl->render_pass.render_pass(), 0, m_impl->framebuffer.framebuffer};
	for (auto const cb : m_impl->framebuffer.secondary) { cb.begin({vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii}); }
	return fill_and_return();
}

Pipeline Renderer::bind_pipeline(vk::CommandBuffer cb, Pipeline::State const& state, std::string const& shader_id) {
	auto const shader = m_impl->shader_db.find(shader_id);
	if (!shader) { throw Error{concat("Failed to find shader: ", shader_id)}; }
	auto ret = m_impl->pipes.get(m_impl->render_pass.render_pass(), state, shader);
	ret.bind(cb);
	glm::vec2 const extent = glm::uvec2{m_impl->render_target->extent.width, m_impl->render_target->extent.height};
	auto viewport = vk::Viewport{0.0f, extent.y, extent.x, -extent.y, 0.0f, 1.0f};
	auto scissor = vk::Rect2D{{}, m_impl->render_target->extent};
	cb.setViewport(0U, viewport);
	cb.setScissor(0U, scissor);
	return ret;
}

bool Renderer::render() {
	if (!m_impl->render_target) { return false; }

	ImGui::Render();
	auto& frame = m_impl->render_frames.get();

	auto const cbs = frame.secondary.span();
	m_impl->dear_imgui.render(cbs.front());
	for (auto cb : cbs) { cb.end(); }

	frame.primary.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	m_impl->render_target->to_draw(frame.primary);

	auto const clear_colour = vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}};
	m_impl->render_pass.execute(m_impl->framebuffer, clear_colour);

	m_impl->render_target->to_present(frame.primary);
	frame.primary.end();

	frame.submit(m_impl->gfx.queue);
	m_impl->swapchain.present(m_impl->window.framebuffer_extent(), *frame.sync.present);

	m_impl->pipes.rotate();
	m_impl->render_frames.rotate();

	m_impl->render_target.reset();
	return true;
}

Shader Renderer::add_shader(std::string id, SpirV vert, SpirV frag) { return m_impl->shader_db.add(std::move(id), std::move(vert), std::move(frag)); }
bool Renderer::add_shader(Shader shader) { return m_impl->shader_db.add(std::move(shader)); }
Shader Renderer::find_shader(std::string const& id) const { return m_impl->shader_db.find(id); }
} // namespace facade
