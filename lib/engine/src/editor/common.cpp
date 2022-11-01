#include <imgui.h>
#include <facade/engine/editor/common.hpp>
#include <cassert>

namespace facade::editor {
Openable::Openable(bool is_open) : m_open(is_open) {}

Window::Window(char const* label, bool* open_if, int flags) : Openable(ImGui::Begin(label, open_if, flags)) {}

Window::Window(NotClosed<Window>, char const* label, glm::vec2 size, Bool border, int flags)
	: Openable(ImGui::BeginChild(label, {size.x, size.y}, border.value, flags)), m_child(true) {}

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
	if (TreeNode{label, flags | ImGuiTreeNodeFlags_Leaf}) { return true; }
	return false;
}

Window::Menu::Menu(NotClosed<Window>) : MenuBar(ImGui::BeginMenuBar()) {}

Window::Menu::~Menu() {
	if (m_open) { ImGui::EndMenuBar(); }
}

MainMenu::MainMenu() : MenuBar(ImGui::BeginMainMenuBar()) {}

MainMenu::~MainMenu() {
	if (m_open) { ImGui::EndMainMenuBar(); }
}

Popup::Popup(char const* id, Bool modal, Bool centered, int flags) {
	if (centered) {
		auto const center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
	}
	if (modal) {
		m_open = ImGui::BeginPopupModal(id, {}, flags);
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
