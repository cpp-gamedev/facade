#include <imgui.h>
#include <imgui_internal.h>
#include <facade/engine/editor/log.hpp>
#include <facade/util/fixed_string.hpp>

namespace facade {
void editor::display_log(NotClosed<Window> window, LogState& out_state) {
	ImGui::Text("%s", FixedString{"Count: {}", out_state.display_count}.c_str());
	ImGui::SameLine();
	float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	ImGui::PushButtonRepeat(true);
	auto const max_logs = static_cast<int>(logger::buffer_size().total());
	if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { out_state.display_count = std::clamp(out_state.display_count - 10, 0, max_logs); }
	ImGui::SameLine(0.0f, spacing);
	if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { out_state.display_count = std::clamp(out_state.display_count + 10, 0, max_logs); }
	ImGui::PopButtonRepeat();

	static constexpr std::string_view levels_v[] = {"Error", "Warn", "Info", "Debug"};
	static constexpr auto max_log_level_v = debug_v ? logger::Level::eDebug : logger::Level::eInfo;
	for (logger::Level l = logger::Level::eError; l <= max_log_level_v; l = static_cast<logger::Level>(static_cast<int>(l) + 1)) {
		ImGui::SameLine();
		ImGui::Checkbox(levels_v[static_cast<std::size_t>(l)].data(), &out_state.level_filter[l]);
	}
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &out_state.auto_scroll);

	ImGui::Separator();
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
	auto span = std::span{out_state.list};
	auto const max_size = static_cast<std::size_t>(out_state.display_count);
	if (span.size() > max_size) { span = span.subspan(span.size() - max_size); }
	if (auto style = editor::StyleVar{ImGuiStyleVar_ItemSpacing, glm::vec2{}}) {
		for (auto const* entry : span) ImGui::TextColored(im_colour(entry->level), "%s", entry->message.c_str());
	}

	if (out_state.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) { ImGui::SetScrollHereY(1.0f); }
}
} // namespace facade
