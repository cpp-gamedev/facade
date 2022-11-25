#include <imgui.h>
#include <facade/engine/editor/scene_tree.hpp>
#include <facade/scene/scene.hpp>

namespace facade::editor {
namespace {
FixedString<128> node_name(Node const& node) {
	auto ret = FixedString<128>{node.name};
	if (ret.empty()) { ret = "(Unnamed)"; }
	ret += FixedString{" ({})", node.self};
	return ret;
}

InspectNode inspect(Node const& node) {
	auto ret = InspectNode{};
	ret.id = node.self;
	ret.name = node_name(node);
	ret.name += FixedString{"###Node"};
	return ret;
}

void walk(Scene& out_scene, Node& node, InspectNode& inout) {
	auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (node.self == inout.id) { flags |= ImGuiTreeNodeFlags_Selected; }
	if (node.children.empty()) {
		flags |= ImGuiTreeNodeFlags_Leaf;
		if (&node == &out_scene.camera() && node.attachments.size() == 1) { return; }
	}
	auto tn = editor::TreeNode{node_name(node).c_str(), flags};
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
		inout = inspect(node);
	} else if ((flags & ImGuiTreeNodeFlags_Selected) && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
		inout = {};
	}
	if (tn) {
		for (auto child : node.children) { walk(out_scene, *out_scene.resources().nodes.find(child), inout); }
	}
}
} // namespace

bool SceneTree::render(NotClosed<Window>, InspectNode& inout) const {
	auto const in = inout.id;
	auto& nodes = m_scene.resources().nodes;
	auto* node = in ? nodes.find(*inout.id) : nullptr;
	if (!node) { inout = {}; }
	for (auto id : m_scene.roots()) { walk(m_scene, *nodes.find(id), inout); }
	return inout.id != in;
}
} // namespace facade::editor
