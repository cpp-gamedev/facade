#include <imgui.h>
#include <facade/engine/editor/inspector.hpp>
#include <facade/engine/editor/reflector.hpp>
#include <facade/engine/editor/resource_browser.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/enumerate.hpp>
#include <facade/util/logger.hpp>
#include <facade/vk/geometry.hpp>

namespace facade::editor {
void ResourceBrowser::display(NotClosed<Window> target, Scene& out_scene) {
	m_scene = &out_scene;
	m_target = &target;
	auto const ri = ResourceInspector{target, out_scene};
	auto& resources = out_scene.resources();
	static constexpr auto flags_v = ImGuiTreeNodeFlags_Framed;
	bool material{}, mesh{}, static_mesh{};
	if (auto tn = TreeNode("Textures", flags_v)) {
		for (auto [texture, id] : enumerate(resources.textures.view())) { ri.view(texture, id); }
	}
	if (auto tn = TreeNode("Static Meshes", flags_v)) {
		for (auto [static_mesh, id] : enumerate(resources.static_meshes.view())) { ri.view(static_mesh, id); }
		if (ImGui::Button("Add...")) { static_mesh = true; }
	}
	if (auto tn = TreeNode("Materials", flags_v)) {
		for (auto [material, id] : enumerate(resources.materials.view())) { ri.edit(material, id); }
		if (ImGui::Button("Add...")) { material = true; }
	}
	if (auto tn = TreeNode{"Meshes", flags_v}) {
		for (auto [mesh, id] : enumerate(resources.meshes.view())) { ri.edit(mesh, id); }
		if (ImGui::Button("Add...")) { mesh = true; }
	}

	add_mesh(mesh);
	add_material(material);
	add_static_mesh(static_mesh);
}

void ResourceBrowser::add_mesh(bool trigger) {
	if (trigger) { Popup::open("Add Mesh"); }
	if (auto popup = Popup{"Add Mesh"}) {
		if (m_data.mesh.buffer.empty()) { m_data.mesh.buffer.resize(128, '\0'); }
		if (trigger) { ImGui::SetKeyboardFocusHere(); }
		ImGui::InputText("Name", m_data.mesh.buffer.data(), m_data.mesh.buffer.size());
		if (ImGui::Button("Add") && *m_data.mesh.buffer.c_str()) {
			m_scene->add(Mesh{.name = m_data.mesh.buffer.c_str()});
			Popup::close_current();
		}
	}
}

void ResourceBrowser::add_material(bool trigger) {
	if (trigger) { Popup::open("Add Material"); }
	if (auto popup = Popup{"Add Material"}) {
		if (m_data.mat.buffer.empty()) { m_data.mat.buffer.resize(128, '\0'); }
		if (trigger) { ImGui::SetKeyboardFocusHere(); }
		ImGui::InputText("Name", m_data.mat.buffer.data(), m_data.mat.buffer.size());
		static constexpr char const* mat_types_v[] = {"Lit", "Unlit"};
		static constexpr auto mat_types_size_v{static_cast<int>(std::size(mat_types_v))};
		char const* mat_type = m_data.mat.type >= 0 && m_data.mat.type < mat_types_size_v ? mat_types_v[m_data.mat.type] : "Invalid";
		ImGui::SliderInt("Type", &m_data.mat.type, 0, mat_types_size_v - 1, mat_type);
		if (ImGui::Button("Add") && *m_data.mat.buffer.c_str()) {
			auto mat = Material{};
			switch (m_data.mat.type) {
			case 1: mat = Material{UnlitMaterial{}}; break;
			default: mat = Material{LitMaterial{}}; break;
			}
			mat.name = m_data.mat.buffer.c_str();
			m_scene->add(std::move(mat));
			Popup::close_current();
		}
	}
}

void ResourceBrowser::add_static_mesh(bool trigger) {
	if (trigger) { Popup::open("Add Static Mesh"); }
	if (auto popup = Popup{"Add Static Mesh"}) {
		if (m_data.static_mesh.buffer.empty()) { m_data.static_mesh.buffer.resize(128, '\0'); }
		auto& sm = m_data.static_mesh;
		auto const reflect = Reflector{*m_target};
		if (trigger) { ImGui::SetKeyboardFocusHere(); }
		ImGui::InputText("Name", sm.buffer.data(), sm.buffer.size());
		ImGui::Separator();
		if (auto tab = TabBar{"Static Mesh"}) {
			if (auto item = TabBar::Item{tab, "Cube"}) {
				reflect("Dimensions", sm.cube.size, 0.05f, 0.01f, Reflector::max_v);
				reflect("Vertex Colour", Reflector::AsRgb{sm.cube.vertex_rgb});
				sm.type = SmType::eCube;
			}
			if (auto item = TabBar::Item{tab, "Sphere"}) {
				ImGui::DragFloat("Diameter", &sm.sphere.diameter, 0.05f, 0.01f, Reflector::max_v);
				reflect("Vertex Colour", Reflector::AsRgb{sm.sphere.vertex_rgb});
				ImGui::SliderInt("Quads per side", &sm.sphere.quads_per_side, 1, 128);
				sm.type = SmType::eSphere;
			}
			auto cylinder = TabBar::Item{tab, "Cylinder"};
			auto cone = TabBar::Item{tab, "Cone"};
			if (cylinder || cone) {
				ImGui::DragFloat("Diameter (xz)", &sm.conic.xz_diam, 0.05f, 0.01f, Reflector::max_v);
				ImGui::DragFloat("Height (y)", &sm.conic.y_height, 0.05f, 0.01f, Reflector::max_v);
				reflect("Vertex Colour", Reflector::AsRgb{sm.conic.vertex_rgb});
				ImGui::SliderInt("Points", &sm.conic.xz_points, 6, 1024);
				sm.type = cylinder ? SmType::eCylinder : SmType::eCone;
			}
			if (auto item = TabBar::Item{tab, "Arrow"}) {
				ImGui::DragFloat("Diameter (xz)", &sm.arrow.stalk_diam, 0.05f, 0.01f, Reflector::max_v);
				ImGui::DragFloat("Height (y)", &sm.arrow.stalk_height, 0.05f, 0.01f, Reflector::max_v);
				reflect("Vertex Colour", Reflector::AsRgb{sm.arrow.vertex_rgb});
				ImGui::SliderInt("Points", &sm.arrow.xz_points, 6, 1024);
				sm.type = SmType::eArrow;
			}
		}
		if (ImGui::Button("Add") && *sm.buffer.c_str()) {
			auto const qps = static_cast<std::uint32_t>(sm.sphere.quads_per_side);
			auto const conic_pts = static_cast<std::uint32_t>(sm.conic.xz_points);
			auto const arrow_pts = static_cast<std::uint32_t>(sm.arrow.xz_points);
			auto geometry = Geometry{};
			switch (sm.type) {
			case SmType::eCube: geometry = make_cube(sm.cube.size, sm.cube.vertex_rgb); break;
			case SmType::eSphere: geometry = make_cubed_sphere(sm.sphere.diameter, qps, sm.sphere.vertex_rgb); break;
			case SmType::eCone: geometry = make_cone(sm.conic.xz_diam, sm.conic.y_height, conic_pts); break;
			case SmType::eCylinder: geometry = make_cylinder(sm.conic.xz_diam, sm.conic.y_height, conic_pts); break;
			case SmType::eArrow: geometry = make_arrow(sm.arrow.stalk_diam, sm.arrow.stalk_height, arrow_pts, sm.arrow.vertex_rgb); break;
			default: logger::warn("Unknown static mesh type"); break;
			}
			if (!geometry.vertices.empty()) { m_scene->add(std::move(geometry), sm.buffer.c_str()); }
			Popup::close_current();
		}
	}
}
} // namespace facade::editor
