#include <imgui.h>
#include <facade/engine/editor/drag_drop_id.hpp>
#include <facade/engine/editor/inspector_old.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/fixed_string.hpp>
#include <facade/util/logger.hpp>
#include <cassert>

namespace facade::editor::old {
namespace {
constexpr glm::vec3 to_degree(glm::vec3 const& angles) { return {glm::degrees(angles.x), glm::degrees(angles.y), glm::degrees(angles.z)}; }

struct Modified {
	bool value{};

	constexpr bool operator()(bool const modified) {
		value |= modified;
		return modified;
	}
};

using DragFloatFunc = bool (*)(char const*, float* v, float, float, float, char const*, ImGuiSliderFlags);

constexpr DragFloatFunc drag_float_vtable[] = {
	nullptr, &ImGui::DragFloat, &ImGui::DragFloat2, &ImGui::DragFloat3, &ImGui::DragFloat4,
};

template <std::size_t Dim>
bool drag_float(char const* label, float (&data)[Dim], float speed, float lo, float hi, int flags = {}) {
	static_assert(Dim > 0 && Dim < std::size(drag_float_vtable));
	return drag_float_vtable[Dim](label, data, speed, lo, hi, "%.3f", flags);
}

template <std::size_t Dim>
bool inspect_vec(char const* label, glm::vec<static_cast<int>(Dim), float>& out_vec, float speed, float lo, float hi) {
	float data[Dim]{};
	std::memcpy(data, &out_vec, Dim * sizeof(float));
	if (drag_float(label, data, speed, lo, hi)) {
		std::memcpy(&out_vec, data, Dim * sizeof(float));
		return true;
	}
	return false;
}
} // namespace

bool Inspector::inspect(char const* label, glm::vec2& out_vec2, float speed, float lo, float hi) const {
	return inspect_vec<2>(label, out_vec2, speed, lo, hi);
}

bool Inspector::inspect(char const* label, glm::vec3& out_vec3, float speed, float lo, float hi) const {
	return inspect_vec<3>(label, out_vec3, speed, lo, hi);
}

bool Inspector::inspect(char const* label, glm::vec4& out_vec4, float speed, float lo, float hi) const {
	return inspect_vec<4>(label, out_vec4, speed, lo, hi);
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
	float deg[3] = {euler.x, euler.y, euler.z};
	auto const org = euler;
	if (drag_float(label, deg, 0.5f, -180.0f, 180.0f, ImGuiSliderFlags_NoInput)) {
		euler = {deg[0], deg[1], deg[2]};
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
	if (ret(inspect("Position", vec3, 0.1f))) { out_transform.set_position(vec3); }
	auto quat = out_transform.orientation();
	if (ret(inspect("Orientation", quat))) { out_transform.set_orientation(quat); }
	vec3 = out_transform.scale();
	if (out_unified_scaling) {
		if (ret(ImGui::DragFloat("Scale", &vec3.x, 0.05f))) { out_transform.set_scale({vec3.x, vec3.x, vec3.x}); }
	} else {
		if (ret(inspect("Scale", vec3, 0.05f))) { out_transform.set_scale(vec3); }
	}
	if (scaling_toggle) {
		ImGui::SameLine();
		ImGui::Checkbox("Unified", &out_unified_scaling.value);
	}
	return ret.value;
}

SceneInspector::SceneInspector(NotClosed<Window> target, Scene& scene) : Inspector(target), m_scene(scene), m_target(target) {}

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
	if (out_material.emissive) { ret(inspect_rgb("Emission", out_material.emissive_factor)); }
	if (out_material.base_colour || out_material.roughness_metallic) {
		auto const ri = ResourceInspector{m_target, m_scene.resources()};
		if (out_material.base_colour) {
			auto const* tex = m_scene.find(*out_material.base_colour);
			ri.display(*tex, *out_material.base_colour, "Base Colour: ");
			if (ImGui::BeginDragDropTarget()) {
				if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("ID_TEXTURE")) {
					assert(payload->DataSize == sizeof(std::size_t));
					out_material.base_colour = *reinterpret_cast<std::size_t*>(payload->Data);
				}
				ImGui::EndDragDropTarget();
			}
		}
		if (out_material.roughness_metallic) {
			auto const* tex = m_scene.find(*out_material.roughness_metallic);
			ri.display(*tex, *out_material.roughness_metallic, "Roughness Metallic: ");
		}
		if (out_material.emissive) {
			auto const* tex = m_scene.find(*out_material.emissive);
			ri.display(*tex, *out_material.emissive, "Emissive: ");
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

	static constexpr char const* inspect_node_names_v[] = {
		"Static Mesh",
		"Material",
	};

	auto const get_inspect_node_offset = [](float existing) {
		static auto ret = max_size(inspect_node_names_v);
		return ret.x + existing;
	};

	if (ImGui::CollapsingHeader(FixedString{"Mesh ({})###Mesh", mesh_id}.c_str())) {
		for (auto [primitive, index] : enumerate(mesh->primitives)) {
			if (auto tn = TreeNode{FixedString{"Primitive [{}]", index}.c_str()}) {
				auto const cursor_offset_x = ImGui::GetCursorPosX() + 20.0f;
				ImGui::Text("Static Mesh");
				ImGui::SameLine(get_inspect_node_offset(cursor_offset_x));
				auto const* static_mesh = m_scene.find(primitive.static_mesh);
				if (TreeNode::leaf(FixedString{"{} ({})", static_mesh->name(), primitive.static_mesh}.c_str())) {
					logger::debug("Resource Window:: focus(Id<StaticMesh> = {})", primitive.static_mesh);
				}
				if (primitive.material) {
					ImGui::Text("Material");
					ImGui::SameLine(get_inspect_node_offset(cursor_offset_x));
					auto const* material = m_scene.find(*primitive.material);
					if (TreeNode::leaf(FixedString{"{} ({})", material->name, *primitive.material}.c_str())) {
						logger::debug("Resource Window:: focus(Id<Material> = {})", *primitive.material);
					}
				}

				ImGui::Separator();
				TreeNode::leaf(FixedString{"Static Mesh ({})", primitive.static_mesh}.c_str());
				if (primitive.material) { ret(inspect(*primitive.material)); }
			}
		}
	}
	return ret.value;
}

bool SceneInspector::inspect(Id<Camera> camera_id) const {
	auto ret = Modified{};
	auto* camera = m_scene.find(camera_id);
	if (!camera) { return ret.value; }
	if (ImGui::CollapsingHeader(FixedString{"Camera ({})###Camera", camera_id}.c_str())) {
		ret(ImGui::DragFloat("Exposure", &camera->exposure, 0.1f, 0.0f, 10.0f));
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
	if (auto const* camera_id = node->find<Id<Camera>>()) { ret(inspect(*camera_id)); }
	return ret.value;
}

namespace {
template <typename Named>
void display_resource(Named const& named, std::size_t const index, std::string_view prefix = {}) {
	editor::TreeNode::leaf(FixedString<128>{"{}{} ({})", prefix, named.name(), index}.c_str());
}
} // namespace

ResourceInspector::ResourceInspector(NotClosed<Window>, Scene::Resources const& resources) : m_resources(resources) {}

void ResourceInspector::display() const {
	static constexpr auto flags_v = ImGuiTreeNodeFlags_Framed;
	if (auto tn = editor::TreeNode("Cameras", flags_v)) {
		for (auto const [camera, index] : enumerate(m_resources.cameras)) { display(camera, index); }
	}
	if (auto tn = editor::TreeNode("Samplers", flags_v)) {
		for (auto const [sampler, index] : enumerate(m_resources.samplers)) { display_resource(sampler, index); }
	}
	if (auto tn = editor::TreeNode("Textures", flags_v)) {
		for (auto const [texture, index] : enumerate(m_resources.textures)) { display(texture, index); }
	}
	if (auto tn = editor::TreeNode("Materials", flags_v)) {
		for (auto const [material, index] : enumerate(m_resources.materials)) { display(*material, index); }
	}
	if (auto tn = editor::TreeNode("Static Meshes", flags_v)) {
		for (auto const [mesh, index] : enumerate(m_resources.static_meshes)) { display_resource(mesh, index); }
	}
	if (auto tn = editor::TreeNode("Meshes", flags_v)) {
		for (auto const [mesh, index] : enumerate(m_resources.meshes)) { display(mesh, index); }
	}
}

void ResourceInspector::display(Camera const& camera, std::size_t index, std::string_view prefix) const {
	if (auto tn = editor::TreeNode{FixedString<128>{"{}{} ({})", prefix, camera.name, index}.c_str()}) {
		auto const visitor = Visitor{
			[](Camera::Orthographic const& o) {
				ImGui::Text("%s", FixedString{"Near Plane: {}", o.view_plane.near}.c_str());
				ImGui::Text("%s", FixedString{"Far Plane: {}", o.view_plane.near}.c_str());
			},
			[](Camera::Perspective const& p) {
				ImGui::Text("%s", FixedString{"FoV: {}", p.field_of_view}.c_str());
				ImGui::Text("%s", FixedString{"Near Plane: {}", p.view_plane.near}.c_str());
				ImGui::Text("%s", FixedString{"Far Plane: {}", p.view_plane.near}.c_str());
			},
		};
		std::visit(visitor, camera.type);
	}
}

void ResourceInspector::display(Texture const& texture, std::size_t index, std::string_view prefix) const {
	auto const name = FixedString<128>{"{}{} ({})", prefix, texture.name(), index};
	auto tn = editor::TreeNode{name.c_str()};
	drag_payload(Id<Texture>{index}, "ID_TEXTURE", name.c_str());
	if (tn) {
		auto const view = texture.view();
		editor::TreeNode::leaf(FixedString{"Extent: {}x{}", view.extent.width, view.extent.height}.c_str());
		editor::TreeNode::leaf(FixedString{"Mip levels: {}", texture.mip_levels()}.c_str());
		auto const cs = texture.colour_space() == ColourSpace::eLinear ? "linear" : "sRGB";
		editor::TreeNode::leaf(FixedString{"Colour Space: {}", cs}.c_str());
	}
}

void ResourceInspector::display(Material const& material, std::size_t const index, std::string_view prefix) const {
	auto const name = FixedString<128>{"{}{} ({})", prefix, material.name, index};
	auto tn = editor::TreeNode{name.c_str()};
	drag_payload(Id<Material>{index}, "ID_MATERIAL", name.c_str());
	if (tn) {
		if (auto const* lit = dynamic_cast<LitMaterial const*>(&material)) { return display(*lit); }
		if (auto const* unlit = dynamic_cast<UnlitMaterial const*>(&material)) { return display(*unlit); }
	}
}

void ResourceInspector::display(Mesh const& mesh, std::size_t const index, std::string_view prefix) const {
	auto const name = FixedString<128>{"{}{} ({})", prefix, mesh.name, index};
	auto tn = editor::TreeNode{name.c_str()};
	drag_payload(Id<Mesh>{index}, "ID_MESH", name.c_str());
	if (tn) {
		for (auto const primitive : mesh.primitives) {
			display_resource(m_resources.static_meshes[primitive.static_mesh], primitive.static_mesh, "Static Mesh: ");
			if (primitive.material) { display(*m_resources.materials[*primitive.material], *primitive.material, "Material: "); }
		}
	}
}

void ResourceInspector::display(LitMaterial const& lit) const {
	ImGui::Text("Albedo: ");
	ImGui::SameLine();
	ImGui::ColorButton("Albedo", {lit.albedo.x, lit.albedo.y, lit.albedo.z, 1.0f});
	ImGui::Text("%s", FixedString{"Metallic: {:.2f}", lit.metallic}.c_str());
	ImGui::Text("%s", FixedString{"Roughness: {:.2f}", lit.roughness}.c_str());
	if (lit.base_colour) { display(m_resources.textures[*lit.base_colour], *lit.base_colour, "Base Colour: "); }
	if (lit.roughness_metallic) { display(m_resources.textures[*lit.roughness_metallic], *lit.roughness_metallic, "Roughness Metallic: "); }
}

void ResourceInspector::display(UnlitMaterial const& unlit) const {
	if (unlit.texture) { display(m_resources.textures[*unlit.texture], *unlit.texture, "Texture: "); }
}
} // namespace facade::editor::old
