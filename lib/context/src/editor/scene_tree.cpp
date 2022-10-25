#include <imgui.h>
#include <facade/context/editor/scene_tree.hpp>
#include <facade/scene/scene.hpp>

namespace facade::editor {
namespace {
FixedString<128> node_name(Node const& node) {
	auto ret = FixedString<128>{node.name};
	if (ret.empty()) { ret = "(Unnamed)"; }
	ret += FixedString{" ({})", node.id()};
	return ret;
}

Inspectee inspect(Node const& node) {
	auto ret = Inspectee{};
	ret.id = node.id();
	ret.name = node_name(node);
	ret.name += FixedString{"###Node"};
	return ret;
}

void walk(Node& node, Inspectee& inout) {
	auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (node.id() == inout.id) { flags |= ImGuiTreeNodeFlags_Selected; }
	if (node.children().empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
	auto tn = editor::TreeNode{node_name(node).c_str(), flags};
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
		inout = inspect(node);
	} else if ((flags & ImGuiTreeNodeFlags_Selected) && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
		inout = {};
	}
	if (tn) {
		for (auto& child : node.children()) { walk(child, inout); }
	}
}
} // namespace

bool SceneTree::render(NotClosed<Window>, Inspectee& inout) const {
	auto const in = inout.id;
	if (!m_scene.find_node(in)) { inout = {}; }
	for (auto& root : m_scene.roots()) { walk(root, inout); }
	return inout.id != in;
}
} // namespace facade::editor