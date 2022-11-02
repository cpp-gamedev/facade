#include <imgui.h>
#include <facade/engine/editor/inspector.hpp>
#include <facade/engine/editor/scene_tree.hpp>
#include <facade/engine/engine.hpp>
#include <facade/render/renderer.hpp>
#include <facade/util/error.hpp>
#include <facade/util/fixed_string.hpp>
#include <facade/util/logger.hpp>
#include <main_menu/main_menu.hpp>

namespace facade {
void WindowMenu::display_menu(editor::NotClosed<editor::MainMenu> main) {
	auto menu = editor::Menu{main, "Window"};
	if (!menu) { return; }

	if (ImGui::MenuItem("Scene")) { m_flags.scene = true; }
	if (ImGui::MenuItem("Frame Stats")) { m_flags.stats = true; }
	if (ImGui::MenuItem("Log")) { m_flags.log = true; }
	if (ImGui::MenuItem("Close All")) { m_flags = {}; }
	ImGui::Separator();
	if (ImGui::MenuItem("Dear ImGui Demo")) { m_flags.demo = true; }
}

void WindowMenu::display_windows(Engine& engine) {
	if (m_flags.scene) { m_flags.scene = display_scene(engine.scene()); }
	if (m_flags.lights) { m_flags.lights = display_lights(engine.scene().lights); }
	if (m_data.inspect && !display_inspector(engine.scene())) { m_data.inspect = {}; }
	if (m_flags.log) { m_flags.log = display_log(); }
	if (m_flags.stats) { m_flags.stats = display_stats(engine); }
}

bool WindowMenu::display_scene(Scene& scene) {
	ImGui::SetNextWindowSize({250.0f, 350.0f}, ImGuiCond_FirstUseEver);
	bool show{true};
	if (auto window = editor::Window{"Scene", &show}) {
		if (ImGui::Button("Lights")) { m_flags.lights = true; }
		ImGui::Separator();
		editor::SceneTree{scene}.render(window, m_data.inspect);
	}
	return show;
}

bool WindowMenu::display_lights(Lights& lights) {
	ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_FirstUseEver);
	bool show{true};
	if (auto window = editor::Window{"Lights", &show}) { editor::Inspector{window}.inspect(lights); }
	return show;
}

bool WindowMenu::display_inspector(Scene& scene) {
	bool show = true;
	ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_FirstUseEver);
	if (auto window = editor::Window{m_data.inspect.name.c_str(), &show}) {
		editor::SceneInspector{window, scene}.inspect(m_data.inspect.id, m_data.unified_scaling);
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
		if (ImGui::SmallButton("Vsync")) { change_vsync(engine); }
		ImGui::SameLine();
		ImGui::Text("%s", vsync_status(stats.mode).data());
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

auto FileMenu::display(editor::NotClosed<editor::MainMenu> main) -> Command {
	auto ret = Command{};
	if (auto file = editor::Menu{main, "File"}) {
		if (!m_recents.empty()) { ret = open_recent(main); }
		ImGui::Separator();
		if (ImGui::MenuItem("Exit")) { ret = Shutdown{}; }
	}
	return ret;
}

auto FileMenu::open_recent(editor::NotClosed<editor::MainMenu> main) -> Command {
	auto ret = Command{};
	if (auto open_recent = editor::Menu{main, "Open Recent"}) {
		for (auto it = m_recents.rbegin(); it != m_recents.rend(); ++it) {
			if (ImGui::MenuItem(Engine::State::to_filename(*it).c_str())) { ret = OpenRecent{.path = *it}; }
		}
	}
	return ret;
}
} // namespace facade
