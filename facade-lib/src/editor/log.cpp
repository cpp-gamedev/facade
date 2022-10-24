#include <imgui.h>
#include <facade/editor/common.hpp>
#include <facade/editor/log.hpp>
#include <facade/util/fixed_string.hpp>

namespace facade::editor {
void Log::render() {
	ImGui::SetNextWindowSize({500.0f, 200.0f}, ImGuiCond_Once);
	if (auto window = editor::Window{"Log", &show}) {
		ImGui::Text("%s", FixedString{"Count: {}", m_display_count}.c_str());
		ImGui::SameLine();
		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { m_display_count = std::clamp(m_display_count - 10, 0, 1000); }
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { m_display_count = std::clamp(m_display_count + 10, 0, 1000); }
		ImGui::PopButtonRepeat();

		static constexpr std::string_view levels_v[] = {"Error", "Warn", "Info", "Debug"};
		static constexpr auto max_log_level_v = debug_v ? logger::Level::eDebug : logger::Level::eInfo;
		for (logger::Level l = logger::Level::eError; l <= max_log_level_v; l = static_cast<logger::Level>(static_cast<int>(l) + 1)) {
			ImGui::SameLine();
			ImGui::Checkbox(levels_v[static_cast<std::size_t>(l)].data(), &m_level_filter[l]);
		}

		logger::access_buffer(*this);
		auto child = editor::Window{window, "scroll", {}, {}, ImGuiWindowFlags_HorizontalScrollbar};
		static constexpr auto im_colour = [](logger::Level const l) {
			switch (l) {
			case logger::Level::eError: return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
			case logger::Level::eWarn: return ImVec4{1.0f, 1.0f, 0.0f, 1.0f};
			default:
			case logger::Level::eInfo: return ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
			case logger::Level::eDebug: return ImVec4{0.5f, 0.5f, 0.5f, 1.0f};
			}
		};
		if (auto style = editor::StyleVar{ImGuiStyleVar_ItemSpacing, glm::vec2{}}) {
			for (auto const* entry : m_list) ImGui::TextColored(im_colour(entry->level), "%s", entry->message.c_str());
		}
	}
}

void Log::operator()(std::span<logger::Entry const> entries) {
	m_list.clear();
	for (auto const& entry : entries) {
		if (m_list.size() >= static_cast<std::size_t>(m_display_count)) { break; }
		if (!m_level_filter[entry.level]) { continue; }
		m_list.push_back(&entry);
	}
}
} // namespace facade::editor
