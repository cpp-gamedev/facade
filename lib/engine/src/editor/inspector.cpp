#include <imgui.h>
#include <facade/engine/editor/drag_drop_id.hpp>
#include <facade/engine/editor/inspector.hpp>
#include <facade/engine/editor/reflector.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/fixed_string.hpp>
#include <facade/util/logger.hpp>

namespace facade::editor {
namespace {
void edit_material(SceneResources const& resources, LitMaterial& lit) {
	ImGui::Text("Albedo: ");
	ImGui::SameLine();
	ImGui::ColorButton("Albedo", {lit.albedo.x, lit.albedo.y, lit.albedo.z, 1.0f});
	ImGui::Text("%s", FixedString{"Metallic: {:.2f}", lit.metallic}.c_str());
	ImGui::Text("%s", FixedString{"Roughness: {:.2f}", lit.roughness}.c_str());

	auto name = FixedString<128>{};
	if (lit.base_colour) { name = FixedString<128>{"{} ({})", resources.textures[*lit.base_colour].name(), *lit.base_colour}; }
	make_id_slot(lit.base_colour, "Base Colour Texture", name.c_str(), "ID_TEXTURE", {true});
	name = {};
	if (lit.roughness_metallic) { name = FixedString<128>{"{} ({})", resources.textures[*lit.roughness_metallic].name(), *lit.roughness_metallic}; }
	make_id_slot(lit.roughness_metallic, "Roughness Metallic", name.c_str(), "ID_TEXTURE", {true});
	name = {};
	if (lit.emissive) { name = FixedString<128>{"{} ({})", resources.textures[*lit.emissive].name(), *lit.emissive}; }
	make_id_slot(lit.emissive, "Emissive", name.c_str(), "ID_TEXTURE", {true});
}

void edit_material(SceneResourcesMut resources, UnlitMaterial& unlit) {
	char const* name = unlit.texture ? resources.textures[*unlit.texture].name().data() : "";
	make_id_slot(unlit.texture, "Texture", name, "ID_TEXTURE", {true});
}
} // namespace

void Inspector::view(Texture const& texture, Id<Texture> id) const {
	auto const name = FixedString<128>{"{} ({})", texture.name(), id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(id, "ID_TEXTURE", name.c_str());
	if (tn) {
		auto const view = texture.view();
		TreeNode::leaf(FixedString{"Extent: {}x{}", view.extent.width, view.extent.height}.c_str());
		TreeNode::leaf(FixedString{"Mip levels: {}", texture.mip_levels()}.c_str());
		auto const cs = texture.colour_space() == ColourSpace::eLinear ? "linear" : "sRGB";
		TreeNode::leaf(FixedString{"Colour Space: {}", cs}.c_str());
	}
}

void Inspector::view(StaticMesh const& mesh, Id<StaticMesh> id) const {
	auto const name = FixedString<128>{"{} ({})", mesh.name(), id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(id, "ID_STATIC_MESH", name.c_str());
	if (tn) {
		ImGui::Text("%s", FixedString{"Vertices: {}", mesh.view().vertex_count}.c_str());
		ImGui::Text("%s", FixedString{"Indices: {}", mesh.view().index_count}.c_str());
	}
}

void Inspector::edit(Material& out_material, Id<Material> id) const {
	auto const name = FixedString<128>{"{} ({})", out_material.name, id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(id, "ID_MATERIAL", name.c_str());
	if (tn) {
		if (auto* lit = dynamic_cast<LitMaterial*>(&out_material)) { return edit_material(m_resources, *lit); }
		if (auto* unlit = dynamic_cast<UnlitMaterial*>(&out_material)) { return edit_material(m_resources, *unlit); }
	}
}

void Inspector::edit(Mesh& out_mesh, Id<Mesh> id) const {
	auto name = FixedString<128>{"{} ({})", m_resources.meshes[id].name, id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(id, "ID_MESH", name.c_str());
	if (tn) {
		for (auto [primitive, index] : enumerate(out_mesh.primitives)) {
			auto tn = TreeNode{FixedString{"Primitive [{}]", index}.c_str()};
			ImGui::SameLine();
			if (small_button_red("x###remove_primitive")) {
				out_mesh.primitives.erase(out_mesh.primitives.begin() + static_cast<std::ptrdiff_t>(index));
				return;
			}
			if (tn) {
				name = FixedString<128>{"{} ({})", m_resources.static_meshes[primitive.static_mesh].name(), primitive.static_mesh};
				make_id_slot(primitive.static_mesh, "Static Mesh", name.c_str(), "ID_STATIC_MESH");

				name = {};
				if (primitive.material) { name = FixedString<128>{"{} ({})", m_resources.materials[*primitive.material]->name, *primitive.material}; }
				make_id_slot(primitive.material, "Material", name.c_str(), "ID_MATERIAL", {true});
			}
		}
		if (ImGui::SmallButton("+###add_primitive")) {
			if (!out_mesh.primitives.empty()) {
				out_mesh.primitives.push_back(out_mesh.primitives.front());
			} else if (!m_resources.static_meshes.empty()) {
				out_mesh.primitives.push_back({});
			}
		}
	}
}

void Inspector::resources() const {
	static constexpr auto flags_v = ImGuiTreeNodeFlags_Framed;
	if (auto tn = TreeNode("Textures", flags_v)) {
		for (auto [texture, id] : enumerate(m_resources.textures)) { view(texture, id); }
	}
	if (auto tn = TreeNode("Static Meshes", flags_v)) {
		for (auto [static_mesh, id] : enumerate(m_resources.static_meshes)) { view(static_mesh, id); }
	}
	if (auto tn = TreeNode("Materials", flags_v)) {
		for (auto [material, id] : enumerate(m_resources.materials)) { edit(*material, id); }
	}
	if (auto tn = TreeNode{"Meshes", flags_v}) {
		for (auto [mesh, id] : enumerate(m_resources.meshes)) { edit(mesh, id); }
	}
}

void Inspector::camera() const {
	auto& node = m_scene.camera();
	auto* id = node.find<Id<Camera>>();
	assert(id && id->value() < m_resources.cameras.size());
	auto& camera = m_resources.cameras[id->value()];
	static constexpr auto flags_v = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed;
	auto const reflect = Reflector{m_target};
	if (auto tn = TreeNode{"Transform", flags_v}) {
		auto vec3 = node.transform.position();
		if (reflect("Position", vec3, 0.1f)) { node.transform.set_position(vec3); }
		auto quat = node.transform.orientation();
		if (reflect("Orientation", quat)) { node.transform.set_orientation(quat); }
	}
	if (auto tn = TreeNode{"Camera", flags_v}) {
		ImGui::DragFloat("Exposure", &camera.exposure, 0.05f, 0.0f, 10.0f);
		auto const visitor = Visitor{
			[](Camera::Orthographic& o) {
				ImGui::DragFloat("Near Plane", &o.view_plane.near);
				ImGui::DragFloat("Far Plane", &o.view_plane.far);
			},
			[](Camera::Perspective& p) {
				auto deg = glm::degrees(p.field_of_view);
				if (ImGui::DragFloat("Field of View", &deg, 0.25f, 20.0f, 90.0f)) { p.field_of_view = glm::radians(deg); }
				ImGui::DragFloat("Near Plane", &p.view_plane.near);
				ImGui::DragFloat("Far Plane", &p.view_plane.far);
			},
		};
		std::visit(visitor, camera.type);
	}
}

void Inspector::lights() const {
	auto to_remove = std::optional<std::size_t>{};
	bool allow_removal = m_scene.lights.dir_lights.size() > 1;
	auto const reflect = Reflector{m_target};
	auto inspect_dir_light = [&](DirLight& out_light, std::size_t index) {
		auto tn = TreeNode{FixedString{"[{}]", index}.c_str()};
		if (allow_removal && small_button_red("x###remove_light")) { to_remove = index; }
		if (tn) {
			reflect("Direction", out_light.direction);
			if (auto tn = TreeNode{"Albedo"}) { reflect(out_light.rgb); }
		}
	};
	if (auto tn = TreeNode{"DirLights", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed}) {
		for (auto [light, index] : enumerate(m_scene.lights.dir_lights.span())) { inspect_dir_light(light, index); }
	}
	if (to_remove) {
		auto replace = decltype(m_scene.lights.dir_lights){};
		for (auto const [light, index] : enumerate(m_scene.lights.dir_lights.span())) {
			if (index == *to_remove) { continue; }
			replace.insert(light);
		}
		m_scene.lights.dir_lights = std::move(replace);
	}
	if (m_scene.lights.dir_lights.size() < Lights::max_lights_v && ImGui::SmallButton("+##add_light")) { m_scene.lights.dir_lights.insert(DirLight{}); }
}

void Inspector::transform(Node& out_node, Bool& out_unified_scaling) const {
	if (auto tn = TreeNode("Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
		Reflector{m_target}(out_node.transform, out_unified_scaling, {true});
	}
}

void Inspector::instances(Node& out_node, Bool unified_scaling) const {
	if (out_node.instances.empty()) { return; }
	if (auto tn = TreeNode{"Instances", ImGuiTreeNodeFlags_Framed}) {
		auto const reflect = Reflector{m_target};
		auto to_remove = std::optional<std::size_t>{};
		for (auto [instance, index] : enumerate(out_node.instances)) {
			auto tn = TreeNode{FixedString{"[{}]", index}.c_str()};
			ImGui::SameLine();
			if (small_button_red("x###remove_instance") && !to_remove) { to_remove = index; }
			if (tn) { reflect(instance, unified_scaling, {false}); }
		}
		if (to_remove) { out_node.instances.erase(out_node.instances.begin() + static_cast<std::ptrdiff_t>(*to_remove)); }
		if (ImGui::SmallButton("+###add_instance")) { out_node.instances.emplace_back(); }
	}
}

void Inspector::mesh(Node& out_node) const {
	auto* mesh_id = out_node.find<Id<Mesh>>();
	if (auto tn = TreeNode{"Mesh", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed}) {
		auto name = FixedString<128>{};
		if (mesh_id) { name = FixedString<128>{"{} ({})", m_scene.find(*mesh_id)->name, *mesh_id}; }
		make_id_slot<Mesh>(out_node, "Mesh", name.c_str(), "ID_MESH");
	}
}

void Inspector::node(Id<Node> node_id, Bool& out_unified_scaling) const {
	auto* node = m_scene.find(node_id);
	if (!node) { return; }
	transform(*node, out_unified_scaling);
	instances(*node, out_unified_scaling);
	mesh(*node);
}
} // namespace facade::editor
