#include <imgui.h>
#include <facade/engine/editor/inspector.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/fixed_string.hpp>
#include <cassert>

namespace facade::editor {
namespace {
bool eq(float const a, float const b, float const epsilon = 0.001f) { return std::abs(a - b) < epsilon; }
constexpr glm::vec3 to_degree(glm::vec3 const& angles) { return {glm::degrees(angles.x), glm::degrees(angles.y), glm::degrees(angles.z)}; }

struct Modified {
	bool value{};

	constexpr bool operator()(bool const modified) {
		value |= modified;
		return modified;
	}
};
} // namespace

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

bool Inspector::inspect(char const* label, nvec3& out_vec3, float speed) const {
	auto vec3 = out_vec3.value();
	if (inspect(label, vec3, speed, -1.0f, 1.0f)) {
		out_vec3 = vec3;
		return true;
	}
	return false;
}

bool Inspector::inspect(char const* label, glm::quat& out_quat) const {
	auto euler = to_degree(glm::eulerAngles(out_quat));
	auto const org = euler;
	if (inspect(label, euler, 0.5f, -180.0f, 180.0f)) {
		if (auto const diff = org.x - euler.x; std::abs(diff) > 0.0f) { out_quat = glm::rotate(out_quat, glm::radians(diff), right_v); }
		if (auto const diff = org.y - euler.y; std::abs(diff) > 0.0f) { out_quat = glm::rotate(out_quat, glm::radians(diff), up_v); }
		if (auto const diff = org.z - euler.z; std::abs(diff) > 0.0f) { out_quat = glm::rotate(out_quat, glm::radians(diff), front_v); }
		return true;
	}
	return false;
}

bool Inspector::inspect_rgb(char const* label, glm::vec3& out_rgb) const {
	float arr[3] = {out_rgb.x, out_rgb.y, out_rgb.z};
	if (ImGui::ColorEdit3(label, arr)) {
		out_rgb = {arr[0], arr[1], arr[2]};
		return true;
	}
	return false;
}

bool Inspector::inspect(char const* label, Rgb& out_rgb) const {
	auto ret = Modified{};
	if (auto tn = TreeNode{label}) {
		auto vec3 = Rgb{.channels = out_rgb.channels, .intensity = 1.0f}.to_vec3();
		if (inspect_rgb("RGB", vec3)) {
			out_rgb = Rgb::make(vec3, out_rgb.intensity);
			ret.value = true;
		}
		ret(ImGui::DragFloat("Intensity", &out_rgb.intensity, 0.05f, 0.1f, 1000.0f));
	}
	return ret.value;
}

bool Inspector::inspect(Transform& out_transform, Bool& out_unified_scaling) const {
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) { return do_inspect(out_transform, out_unified_scaling, {true}); }
	return false;
}

bool Inspector::inspect(std::span<Transform> out_instances, Bool unified_scaling) const {
	if (out_instances.empty()) { return false; }
	auto ret = Modified{};
	if (ImGui::CollapsingHeader("Instances")) {
		for (auto [transform, index] : enumerate(out_instances)) {
			if (auto tn = TreeNode{FixedString{"Instance [{}]", index}.c_str()}) { ret(do_inspect(transform, unified_scaling, {false})); }
		}
	}
	return ret.value;
}

bool Inspector::inspect(Lights& out_lights) const {
	if (out_lights.dir_lights.empty()) { return false; }
	auto ret = Modified{};
	auto to_remove = std::optional<std::size_t>{};
	bool allow_removal = out_lights.dir_lights.size() > 1;
	auto inspect_dir_light = [&](DirLight& out_light, std::size_t index) {
		auto tn = TreeNode{FixedString{"[{}]", index}.c_str()};
		if (allow_removal) {
			ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 10.0f);
			static constexpr auto colour_v = ImVec4{0.8f, 0.0f, 0.1f, 1.0f};
			ImGui::PushStyleColor(ImGuiCol_Button, colour_v);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colour_v);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, colour_v);
			if (ImGui::SmallButton("x")) { to_remove = index; }
			ImGui::PopStyleColor(3);
		}
		if (tn) {
			ret(inspect("Direction", out_light.direction));
			ret(inspect("Albedo", out_light.rgb));
		}
	};
	if (ImGui::CollapsingHeader("DirLights")) {
		for (auto [light, index] : enumerate(out_lights.dir_lights.span())) { inspect_dir_light(light, index); }
	}
	if (to_remove) {
		auto replace = decltype(out_lights.dir_lights){};
		for (auto const [light, index] : enumerate(out_lights.dir_lights.span())) {
			if (index == *to_remove) { continue; }
			replace.insert(light);
		}
		out_lights.dir_lights = std::move(replace);
	}
	if (out_lights.dir_lights.size() < Lights::max_lights_v && ImGui::Button("[+]")) { out_lights.dir_lights.insert(DirLight{}); }
	return ret.value;
}

bool Inspector::do_inspect(Transform& out_transform, Bool& out_unified_scaling, Bool scaling_toggle) const {
	auto ret = Modified{};
	auto vec3 = out_transform.position();
	if (ret(inspect("Position", vec3))) { out_transform.set_position(vec3); }
	auto quat = out_transform.orientation();
	if (ret(inspect("Orientation", quat))) { out_transform.set_orientation(quat); }
	vec3 = out_transform.scale();
	if (out_unified_scaling) {
		if (ret(ImGui::DragFloat("Scale", &vec3.x, 0.1f))) { out_transform.set_scale({vec3.x, vec3.x, vec3.x}); }
	} else {
		if (ret(inspect("Scale", vec3, 0.1f))) { out_transform.set_scale(vec3); }
	}
	if (scaling_toggle) {
		ImGui::SameLine();
		ImGui::Checkbox("Unified", &out_unified_scaling.value);
	}
	return ret.value;
}

SceneInspector::SceneInspector(NotClosed<Window> target, Scene& scene) : Inspector(target), m_scene(scene) {}

bool SceneInspector::inspect(NotClosed<TreeNode>, UnlitMaterial& out_material) const {
	auto ret = Modified{};
	ret(inspect("Tint", out_material.tint, 0.01f, 0.0f, 1.0f));
	return ret.value;
}

bool SceneInspector::inspect(NotClosed<TreeNode>, LitMaterial& out_material) const {
	auto ret = Modified{};
	ret(ImGui::SliderFloat("Metallic", &out_material.metallic, 0.0f, 1.0f));
	ret(ImGui::SliderFloat("Roughness", &out_material.roughness, 0.0f, 1.0f));
	ret(inspect_rgb("Albedo", out_material.albedo));
	if (out_material.base_colour || out_material.roughness_metallic) {
		if (auto tn = TreeNode{"Textures"}) {
			if (out_material.base_colour) {
				auto const* tex = m_scene.find(*out_material.base_colour);
				TreeNode::leaf(FixedString{"Albedo: {} ({})", tex->name, *out_material.base_colour}.c_str());
			}
			if (out_material.roughness_metallic) {
				auto const* tex = m_scene.find(*out_material.roughness_metallic);
				TreeNode::leaf(FixedString{"Roughness-Metallic: {} ({})", tex->name, *out_material.roughness_metallic}.c_str());
			}
		}
	}
	return ret.value;
}

bool SceneInspector::inspect(Id<Material> material_id) const {
	auto* material = m_scene.find(material_id);
	if (!material) { return false; }
	if (auto* unlit = dynamic_cast<UnlitMaterial*>(material)) {
		if (auto tn = TreeNode{FixedString{"Material (Unlit) ({})###Material", material_id}.c_str()}) { return inspect(tn, *unlit); }
		return false;
	} else if (auto* lit = dynamic_cast<LitMaterial*>(material)) {
		if (auto tn = TreeNode{FixedString{"Material (Lit) ({})###Material", material_id}.c_str()}) { return inspect(tn, *lit); }
		return false;
	}
	return false;
}

bool SceneInspector::inspect(Id<Mesh> mesh_id) const {
	auto ret = Modified{};
	auto* mesh = m_scene.find(mesh_id);
	if (!mesh) { return ret.value; }
	if (ImGui::CollapsingHeader(FixedString{"Mesh ({})###Mesh", mesh_id}.c_str())) {
		for (auto [primitive, index] : enumerate(mesh->primitives)) {
			if (auto tn = TreeNode{FixedString{"Primitive [{}]", index}.c_str()}) {
				TreeNode::leaf(FixedString{"Static Mesh ({})", primitive.static_mesh}.c_str());
				if (primitive.material) { ret(inspect(*primitive.material)); }
			}
		}
	}
	return ret.value;
}

bool SceneInspector::inspect(Id<Node> node_id, Bool& out_unified_scaling) const {
	auto ret = Modified{};
	auto* node = m_scene.find(node_id);
	if (!node) { return false; }
	ret(inspect(node->transform, out_unified_scaling));
	ret(inspect(node->instances, out_unified_scaling));
	if (auto const* mesh_id = node->find<Id<Mesh>>()) { ret(inspect(*mesh_id)); }
	return ret.value;
}
} // namespace facade::editor
