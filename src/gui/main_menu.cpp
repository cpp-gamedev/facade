#include <imgui.h>
#include <events/events.hpp>
#include <facade/engine/editor/inspector.hpp>
#include <facade/engine/editor/scene_tree.hpp>
#include <facade/engine/engine.hpp>
#include <facade/render/renderer.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/env.hpp>
#include <facade/util/error.hpp>
#include <facade/util/fixed_string.hpp>
#include <facade/util/logger.hpp>
#include <gui/main_menu.hpp>

namespace facade {
void WindowMenu::display_menu(editor::NotClosed<editor::MainMenu> main) {
	auto menu = editor::Menu{main, "Window"};
	if (!menu) { return; }

	if (ImGui::MenuItem("Scene")) { m_flags.scene = true; }
	if (ImGui::MenuItem("Frame Stats")) { m_flags.stats = true; }
	if (ImGui::MenuItem("Log")) { m_flags.log = true; }
	if (ImGui::MenuItem("Resources")) { m_flags.resources = true; }
	if (ImGui::MenuItem("Close All")) {
		m_flags = {};
		m_data.inspect = {};
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Dear ImGui Demo")) { m_flags.demo = true; }
}

void WindowMenu::display_windows(Engine& engine) {
	if (m_flags.scene) { m_flags.scene = display_scene(engine.scene()); }
	if (m_flags.camera) { m_flags.camera = display_camera(engine.scene()); }
	if (m_flags.lights) { m_flags.lights = display_lights(engine.scene()); }
	if (m_data.inspect && !display_inspector(engine.scene())) { m_data.inspect = {}; }
	if (m_flags.log) { m_flags.log = display_log(); }
	if (m_flags.resources) { m_flags.resources = display_resources(engine.scene()); }
	if (m_flags.stats) { m_flags.stats = display_stats(engine); }
	if (m_flags.demo) { ImGui::ShowDemoWindow(&m_flags.demo); }
}

bool WindowMenu::display_scene(Scene& scene) {
	ImGui::SetNextWindowSize({250.0f, 350.0f}, ImGuiCond_FirstUseEver);
	bool show{true};
	if (auto window = editor::Window{"Scene", &show}) {
		if (ImGui::Button("Lights")) { m_flags.lights = true; }
		ImGui::SameLine();
		if (ImGui::Button("Camera")) { m_flags.camera = true; }
		ImGui::Separator();
		editor::SceneTree{scene}.render(window, m_data.inspect);
	}
	return show;
}

bool WindowMenu::display_camera(Scene& scene) {
	ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_FirstUseEver);
	bool show{true};
	if (auto window = editor::Window{"Camera", &show}) { editor::SceneInspector{window, scene}.camera(); }
	return show;
}

bool WindowMenu::display_lights(Scene& scene) {
	ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_FirstUseEver);
	bool show{true};
	if (auto window = editor::Window{"Lights", &show}) { editor::SceneInspector{window, scene}.lights(); }
	return show;
}

bool WindowMenu::display_inspector(Scene& scene) {
	bool show = true;
	ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_FirstUseEver);
	if (auto window = editor::Window{m_data.inspect.name.c_str(), &show}) {
		editor::SceneInspector{window, scene}.node(m_data.inspect.id, m_data.unified_scaling);
	}
	return m_data.inspect && show;
}

bool WindowMenu::display_stats(Engine& engine) {
	ImGui::SetNextWindowSize({250.0f, 200.0f}, ImGuiCond_FirstUseEver);
	bool show{true};
	if (auto window = editor::Window{"Frame Stats", &show}) {
		auto const& stats = engine.renderer().frame_stats();
		ImGui::Text("%s", FixedString{"Counter: {}", stats.frame_counter}.c_str());
		ImGui::Text("%s", FixedString{"Triangles: {}", stats.triangles}.c_str());
		ImGui::Text("%s", FixedString{"Draw calls: {}", stats.draw_calls}.c_str());
		ImGui::Text("%s", FixedString{"FPS: {}", (stats.fps == 0 ? static_cast<std::uint32_t>(stats.frame_counter) : stats.fps)}.c_str());
		ImGui::Text("%s", FixedString{"Frame time: {:.2f}ms", engine.state().dt * 1000.0f}.c_str());
		ImGui::Text("%s", FixedString{"GPU: {}", stats.gpu_name}.c_str());
		ImGui::Text("%s", FixedString{"MSAA: {}x", to_int(stats.msaa)}.c_str());
		ImGui::Text("%s", FixedString{"Load threads: {}", engine.load_thread_count()}.c_str());
		if (ImGui::SmallButton("Vsync")) { change_vsync(engine); }
		ImGui::SameLine();
		ImGui::Text("%s", vsync_status(stats.mode).data());
		auto& ps = engine.scene().pipeline_state;
		bool wireframe = ps.mode == vk::PolygonMode::eLine;
		if (ImGui::Checkbox("Wireframe", &wireframe)) { ps.mode = wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill; }
		if (wireframe) {
			ImGui::SameLine();
			ImGui::DragFloat("Line Width", &ps.line_width, 0.1f, 1.0f, 10.0f);
		}
	}
	return show;
}

bool WindowMenu::display_log() {
	bool show{true};
	ImGui::SetNextWindowSize({600.0f, 200.0f}, ImGuiCond_FirstUseEver);
	if (auto window = editor::Window{"Log", &show}) {
		logger::access_buffer(*this);
		editor::display_log(window, m_data.log_state);
	}
	return show;
}

bool WindowMenu::display_resources(Scene& scene) {
	bool show{true};
	ImGui::SetNextWindowSize({600.0f, 200.0f}, ImGuiCond_FirstUseEver);
	if (auto window = editor::Window{"Resources", &show}) { editor::SceneInspector{window, scene}.resources(m_data.name_buf); }
	return show;
}

void WindowMenu::change_vsync(Engine const& engine) const {
	static constexpr vk::PresentModeKHR modes[] = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eFifoRelaxed, vk::PresentModeKHR::eMailbox,
												   vk::PresentModeKHR::eImmediate};
	static std::size_t index{0};
	auto const next_mode = [&] {
		while (true) {
			index = (index + 1) % std::size(modes);
			auto const ret = modes[index];
			if (!engine.renderer().is_supported(ret)) { continue; }
			return ret;
		}
		throw Error{"Invariant violated"};
	}();
	engine.renderer().request_mode(next_mode);
	logger::info("Requesting present mode: [{}]", present_mode_str(next_mode));
}

void WindowMenu::operator()(std::span<logger::Entry const> entries) {
	m_data.log_state.list.clear();
	for (auto const& entry : entries) {
		if (!m_data.log_state.level_filter[entry.level]) { continue; }
		m_data.log_state.list.push_back(&entry);
	}
}

void FileMenu::add_recent(std::string path) {
	if (auto it = std::find(m_recents.begin(), m_recents.end(), path); it != m_recents.end()) { m_recents.erase(it); }
	m_recents.push_back(std::move(path));
	max_recents = std::max(max_recents, std::uint8_t{1});
	auto const trim = static_cast<std::ptrdiff_t>(m_recents.size()) - static_cast<std::ptrdiff_t>(max_recents);
	if (trim > 0) {
		std::rotate(m_recents.begin(), m_recents.begin() + trim, m_recents.end());
		m_recents.erase(m_recents.begin() + static_cast<std::ptrdiff_t>(max_recents), m_recents.end());
	}
}

void FileMenu::display(Events const& events, editor::NotClosed<editor::MainMenu> main, Bool loading) {
	if (auto file = editor::Menu{main, "File"}) {
		if (ImGui::MenuItem("Open...", {}, false, !loading)) { events.dispatch(event::OpenFile{}); }
		if (!m_recents.empty()) { open_recent(events, main, loading); }
		ImGui::Separator();
		if (ImGui::MenuItem("Exit")) { events.dispatch(event::Shutdown{}); }
	}
}

void FileMenu::open_recent(Events const& events, editor::NotClosed<editor::MainMenu> main, Bool loading) {
	if (auto open_recent = editor::Menu{main, "Open Recent"}) {
		for (auto it = m_recents.rbegin(); it != m_recents.rend(); ++it) {
			if (ImGui::MenuItem(env::to_filename(*it).c_str(), {}, false, !loading)) { events.dispatch(event::OpenRecent{*it}); }
		}
	}
}
} // namespace facade
