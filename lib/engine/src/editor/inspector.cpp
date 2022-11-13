#include <imgui.h>
#include <facade/engine/editor/drag_drop_id.hpp>
#include <facade/engine/editor/inspector.hpp>
#include <facade/engine/editor/reflector.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/fixed_string.hpp>
#include <facade/util/logger.hpp>

namespace facade::editor {
namespace {
void display(Texture const& texture, Id<Texture> id) {
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

void display(SceneResources resources, LitMaterial& lit) {
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

void display(SceneResourcesMut resources, UnlitMaterial& unlit) {
	char const* name = unlit.texture ? resources.textures[*unlit.texture].name().data() : "";
	make_id_slot(unlit.texture, "Texture", name, "ID_TEXTURE", {true});
}

void display(SceneResourcesMut resources, Material& out_material, Id<Material> id) {
	auto const name = FixedString<128>{"{} ({})", out_material.name, id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(id, "ID_MATERIAL", name.c_str());
	if (tn) {
		if (auto* lit = dynamic_cast<LitMaterial*>(&out_material)) { return display(resources, *lit); }
		if (auto* unlit = dynamic_cast<UnlitMaterial*>(&out_material)) { return display(resources, *unlit); }
	}
}

void display(StaticMesh& out_mesh, Id<StaticMesh> id) {
	auto const name = FixedString<128>{"{} ({})", out_mesh.name(), id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(id, "ID_STATIC_MESH", name.c_str());
	if (tn) {
		ImGui::Text("%s", FixedString{"Vertices: {}", out_mesh.view().vertex_count}.c_str());
		ImGui::Text("%s", FixedString{"Indices: {}", out_mesh.view().index_count}.c_str());
	}
}

void display(SceneResourcesMut resources, Mesh& out_mesh, Id<Mesh> mesh_id) {
	auto name = FixedString<128>{"{} ({})", resources.meshes[mesh_id].name, mesh_id};
	auto tn = TreeNode{name.c_str()};
	drag_payload(mesh_id, "ID_MESH", name.c_str());
	if (tn) {
		for (auto [primitive, index] : enumerate(out_mesh.primitives)) {
			if (auto tn = TreeNode{FixedString{"Primitive [{}]", index}.c_str()}) {
				name = FixedString<128>{"{} ({})", resources.static_meshes[primitive.static_mesh].name(), primitive.static_mesh};
				make_id_slot(primitive.static_mesh, "Static Mesh", name.c_str(), "ID_STATIC_MESH");

				name = {};
				if (primitive.material) { name = FixedString<128>{"{} ({})", resources.materials[*primitive.material]->name, *primitive.material}; }
				make_id_slot(primitive.material, "Material", name.c_str(), "ID_MATERIAL", {true});
			}
		}
	}
}
} // namespace

void Inspector::resources() const {
	static constexpr auto flags_v = ImGuiTreeNodeFlags_Framed;
	auto resources = m_scene.resources();
	if (auto tn = TreeNode("Textures", flags_v)) {
		for (auto [texture, id] : enumerate(resources.textures)) { display(texture, id); }
	}
	if (auto tn = TreeNode("Materials", flags_v)) {
		for (auto [material, id] : enumerate(resources.materials)) { display(resources, *material, id); }
	}
	if (auto tn = TreeNode("Static Meshes", flags_v)) {
		for (auto [static_mesh, id] : enumerate(resources.static_meshes)) { display(static_mesh, id); }
	}
	if (auto tn = TreeNode{"Meshes", flags_v}) {
		for (auto [mesh, id] : enumerate(resources.meshes)) { display(resources, mesh, id); }
	}
}

void Inspector::transform(Node& out_node, Bool& out_unified_scaling) const {
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) { Reflector{m_target}(out_node.transform, out_unified_scaling, {true}); }
}

void Inspector::instances(Node& out_node, Bool unified_scaling) const {
	if (!out_node.instances.empty() && ImGui::CollapsingHeader("Instances")) {
		for (auto [instance, index] : enumerate(out_node.instances)) {
			if (auto tn = TreeNode{FixedString{"[{}]", index}.c_str()}) { Reflector{m_target}(instance, unified_scaling, {false}); }
		}
	}
}

void Inspector::mesh(Node& out_node) const {
	auto* mesh_id = out_node.find<Id<Mesh>>();
	if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
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
