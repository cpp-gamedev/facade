#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/scene/scene.hpp>

namespace facade::editor {
class Inspector {
  public:
	Inspector(NotClosed<Window> target, Scene& out_scene) : m_target(target), m_scene(out_scene), m_resources(out_scene.resources()) {}

	void view(Texture const& texture, Id<Texture> id) const;
	void view(StaticMesh const& mesh, Id<StaticMesh> id) const;

	void edit(Material& out_material, Id<Material> id) const;
	void edit(Mesh& out_mesh, Id<Mesh> id) const;

	void resources(std::string& out_name_buf) const;
	void camera() const;
	void lights() const;
	void transform(Node& out_node, Bool& out_unified_scaling) const;
	void instances(Node& out_node, Bool unified_scaling) const;
	void mesh(Node& out_node) const;
	void node(Id<Node> node_id, Bool& out_unified_scaling) const;

  private:
	NotClosed<Window> m_target;
	Scene& m_scene;
	SceneResourcesMut m_resources;
};
} // namespace facade::editor
