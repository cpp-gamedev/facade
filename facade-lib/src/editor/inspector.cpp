#include <imgui.h>
#include <facade/editor/inspector.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/fixed_string.hpp>
#include <cassert>

namespace facade::editor {
namespace {
bool eq(float const a, float const b, float const epsilon = 0.001f) { return std::abs(a - b) < epsilon; }
constexpr glm::vec3 to_radian(glm::vec3 const& angles) { return {glm::radians(angles.x), glm::radians(angles.y), glm::radians(angles.z)}; }
constexpr glm::vec3 to_degree(glm::vec3 const& angles) { return {glm::degrees(angles.x), glm::degrees(angles.y), glm::degrees(angles.z)}; }

struct Modified {
	bool value{};

	constexpr bool operator()(bool const modified) {
		value |= modified;
		return modified;
	}
};
} // namespace

Inspector::Inspector(Window const& target) { assert(target.is_open()); }

bool Inspector::inspect(char const* label, glm::vec2& out_vec2, float speed, float lo, float hi) const {
	float arr[2] = {out_vec2.x, out_vec2.y};
	ImGui::DragFloat2(label, arr, speed, lo, hi);
	if (!eq(arr[0], out_vec2.x) || !eq(arr[1], out_vec2.y)) {
		out_vec2 = {arr[0], arr[1]};
		return true;
	}
	return false;
}

bool Inspector::inspect(char const* label, glm::vec3& out_vec3, float speed, float lo, float hi) const {
	float arr[3] = {out_vec3.x, out_vec3.y, out_vec3.z};
	ImGui::DragFloat3(label, arr, speed, lo, hi);
	if (!eq(arr[0], out_vec3.x) || !eq(arr[1], out_vec3.y) || !eq(arr[2], out_vec3.z)) {
		out_vec3 = {arr[0], arr[1], arr[2]};
		return true;
	}
	return false;
}

bool Inspector::inspect(char const* label, glm::vec4& out_vec4, float speed, float lo, float hi) const {
	float arr[4] = {out_vec4.x, out_vec4.y, out_vec4.z, out_vec4.w};
	ImGui::DragFloat4(label, arr, speed, lo, hi);
	if (!eq(arr[0], out_vec4.x) || !eq(arr[1], out_vec4.y) || !eq(arr[2], out_vec4.z) || !eq(arr[3], out_vec4.w)) {
		out_vec4 = {arr[0], arr[1], arr[2], arr[3]};
		return true;
	}
	return false;
}

bool Inspector::inspect(char const* label, glm::quat& out_quat) const {
	auto euler = to_degree(glm::eulerAngles(out_quat));
	if (inspect(label, euler, 0.5f, -180.0f, 180.0f)) {
		out_quat = glm::quat(to_radian(euler));
		return true;
	}
	return false;
}

bool Inspector::inspect(Transform& out_transform) const {
	auto ret = Modified{};
	if (auto tn = TreeNode{"Transform"}) {
		auto vec3 = out_transform.position();
		if (ret(inspect("Position", vec3))) { out_transform.set_position(vec3); }
		auto quat = out_transform.orientation();
		if (ret(inspect("Orientation", quat))) { out_transform.set_orientation(quat); }
		vec3 = out_transform.scale();
		if (ret(inspect("Scale", vec3))) { out_transform.set_scale(vec3); }
	}
	return ret.value;
}

SceneInspector::SceneInspector(Window const& target, Scene& scene) : Inspector(target), m_scene(scene) { assert(target.is_open()); }

bool SceneInspector::inspect(TreeNode const& node, UnlitMaterial& out_material) const {
	assert(node.is_open());
	auto ret = Modified{};
	ret(inspect("Tint", out_material.tint, 0.01f, 0.0f, 1.0f));
	return ret.value;
}

bool SceneInspector::inspect(TreeNode const& node, LitMaterial& out_material) const {
	assert(node.is_open());
	auto ret = Modified{};
	ret(ImGui::SliderFloat("Metallic", &out_material.metallic, 0.0f, 1.0f));
	ret(ImGui::SliderFloat("Roughness", &out_material.roughness, 0.0f, 1.0f));
	return ret.value;
}

bool SceneInspector::inspect(Id<Material> material_id) const {
	auto* material = m_scene.find_material(material_id);
	if (!material) { return false; }
	if (auto* unlit = dynamic_cast<UnlitMaterial*>(material)) {
		if (auto tn = TreeNode{FixedString{"Material (Unlit) ({})", material_id}.c_str()}) { return inspect(tn, *unlit); }
		return false;
	} else if (auto* lit = dynamic_cast<LitMaterial*>(material)) {
		if (auto tn = TreeNode{FixedString{"Material (Lit) ({})", material_id}.c_str()}) { return inspect(tn, *lit); }
		return false;
	}
	return false;
}

bool SceneInspector::inspect(Id<Node> node_id) const {
	auto ret = Modified{};
	auto* node = m_scene.find_node(node_id);
	if (!node) { return false; }
	ret(inspect(node->transform));
	if (auto const* mesh_id = node->find<Id<Mesh>>()) { ret(inspect(*mesh_id)); }
	return ret.value;
}

bool SceneInspector::inspect(Id<Mesh> mesh_id) const {
	auto ret = Modified{};
	auto* mesh = m_scene.find_mesh(mesh_id);
	if (!mesh) { return ret.value; }
	if (auto tn = TreeNode{FixedString{"Mesh ({})", mesh_id}.c_str()})
		for (std::size_t i = 0; i < mesh->primitives.size(); ++i) {
			if (auto tn = TreeNode{FixedString{"Primitive {}", i}.c_str()}) {
				auto const& primitive = mesh->primitives[i];
				ImGui::Text("%s", FixedString{"Static Mesh ({})", primitive.static_mesh}.c_str());
				if (primitive.material) { ret(inspect(*primitive.material)); }
			}
		}
	return ret.value;
}
} // namespace facade::editor
