#include <imgui.h>
#include <facade/editor/common.hpp>

namespace facade::editor {
Window::Window(char const* label, bool* open_if, int flags) : m_open(ImGui::Begin(label, open_if, flags)) {}

// ImGui windows requires End() even if Begin() returned false
Window::~Window() { ImGui::End(); }

TreeNode::TreeNode(char const* label, int flags) : m_open(ImGui::TreeNodeEx(label, flags)) {}

TreeNode::~TreeNode() {
	if (m_open) { ImGui::TreePop(); }
}
} // namespace facade::editor
