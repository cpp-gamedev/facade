#include <imgui.h>
#include <facade/engine/editor/common.hpp>
#include <cassert>

namespace facade::editor {
glm::vec2 max_size(std::span<char const* const> strings) {
	auto ret = glm::vec2{};
	for (auto const* str : strings) {
		auto const size = ImGui::CalcTextSize(str);
		ret.x = std::max(ret.x, size.x);
		ret.y = std::max(ret.y, size.y);
	}
	return ret;
}

bool small_button_red(char const* label) {
	bool ret = false;
	ImGui::PushStyleColor(ImGuiCol_Button, {0.8f, 0.0f, 0.1f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.0f, 0.0f, 0.1f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.6f, 0.0f, 0.1f, 1.0f});
	if (ImGui::SmallButton(label)) { ret = true; }
	ImGui::PopStyleColor(3);
	return ret;
}

Openable::Openable(bool is_open) : m_open(is_open) {}

Window::Window(char const* label, bool* open_if, int flags) : Canvas(ImGui::Begin(label, open_if, flags)) {}

Window::Window(NotClosed<Canvas>, char const* label, glm::vec2 size, Bool border, int flags)
	: Canvas(ImGui::BeginChild(label, {size.x, size.y}, border.value, flags)), m_child(true) {}

// ImGui windows requires End() even if Begin() returned false
Window::~Window() {
	if (m_child) {
		ImGui::EndChild();
	} else {
		ImGui::End();
	}
}

TreeNode::TreeNode(char const* label, int flags) : Openable(ImGui::TreeNodeEx(label, flags)) {}

TreeNode::~TreeNode() {
	if (m_open) { ImGui::TreePop(); }
}

bool TreeNode::leaf(char const* label, int flags) {
	auto tn = TreeNode{label, flags | ImGuiTreeNodeFlags_Leaf};
	return ImGui::IsItemClicked();
}

Window::Menu::Menu(NotClosed<Canvas>) : MenuBar(ImGui::BeginMenuBar()) {}

Window::Menu::~Menu() {
	if (m_open) { ImGui::EndMenuBar(); }
}

MainMenu::MainMenu() : MenuBar(ImGui::BeginMainMenuBar()) {}

MainMenu::~MainMenu() {
	if (m_open) { ImGui::EndMainMenuBar(); }
}

Popup::Popup(char const* id, Bool modal, Bool closeable, Bool centered, int flags) {
	if (centered) {
		auto const center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
	}
	if (modal) {
		m_open = ImGui::BeginPopupModal(id, &closeable.value, flags);
	} else {
		m_open = ImGui::BeginPopup(id, flags);
	}
}

Popup::~Popup() {
	if (m_open) { ImGui::EndPopup(); }
}

void Popup::open(char const* id) { ImGui::OpenPopup(id); }

void Popup::close_current() { ImGui::CloseCurrentPopup(); }

Menu::Menu(NotClosed<MenuBar>, char const* label, Bool enabled) : Openable(ImGui::BeginMenu(label, enabled.value)) {}

Menu::~Menu() {
	if (m_open) { ImGui::EndMenu(); }
}

StyleVar::StyleVar(int index, glm::vec2 value) { ImGui::PushStyleVar(index, {value.x, value.y}); }
StyleVar::StyleVar(int index, float value) { ImGui::PushStyleVar(index, value); }
StyleVar::~StyleVar() { ImGui::PopStyleVar(); }
} // namespace facade::editor
