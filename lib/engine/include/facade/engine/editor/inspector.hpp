#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/scene/scene.hpp>

namespace facade::editor {
///
/// \brief Base class for Inspectors.
///
/// Inspectors are stateless ImGui helpers to view / edit various components, with drag-and-drop support.
///
class Inspector {
  protected:
	///
	/// \brief Construct an Inspector instance.
	///
	/// Inspectors don't do anything on construction, constructors exist to enforce invariants instance-wide.
	/// For all Inspectors, an existing Window target is required, Inspector instances will not create any.
	///
	Inspector(NotClosed<Window> target, Scene& out_scene) : m_target(target), m_scene(out_scene), m_resources(out_scene.resources()) {}

	NotClosed<Window> m_target;
	Scene& m_scene;
	SceneResourcesMut m_resources;
};

///
/// \brief Inspector for resources in a Scene.
///
/// Provides drag source (and target) for each resource viewed (and edited).
///
class ResourceInspector : public Inspector {
  public:
	ResourceInspector(NotClosed<Window> target, Scene& out_scene) : Inspector(target, out_scene) {}

	///
	/// \brief Inspect a Texture (read-only).
	/// \param texture Texture to inspect
	/// \param id Id of the Texture being inspected (used to create unique labels and drag payloads)
	///
	void view(Texture const& texture, Id<Texture> id) const;
	///
	/// \brief Inspect a StaticMesh (read-only).
	/// \param mesh the StaticMesh to inspect
	/// \param id Id of the StaticMesh being inspected (used to create unique labels and drag payloads)
	///
	void view(StaticMesh const& mesh, Id<StaticMesh> id) const;

	///
	/// \brief Inspect a Material.
	/// \param out_material Material to inspect
	/// \param id Id of the Material being inspected (used to create unique labels and drag payloads)
	///
	void edit(Material& out_material, Id<Material> id) const;
	///
	/// \brief Inspect a Mesh.
	/// \param out_mesh Mesh to inspect
	/// \param id Id of the Mesh being inspected (used to create unique labels and drag payloads)
	///
	void edit(Mesh& out_mesh, Id<Mesh> id) const;
};

///
/// \brief Inspector for Scene and its Nodes.
///
/// Provides drag-and-drop support where applicable.
/// Uses a framed TreeNode for each top level component.
///
class SceneInspector : public Inspector {
  public:
	SceneInspector(NotClosed<Window> target, Scene& out_scene) : Inspector(target, out_scene) {}

	///
	/// \brief View/edit all resources.
	/// \param out_name_buf Persistent buffer for popups
	///
	/// Uses ResourceInspector.
	///
	void resources(std::string& out_name_buf) const;
	///
	/// \brief Inspect the Scene's camera.
	///
	void camera() const;
	///
	/// \brief Inspect the Scene's lights.
	///
	void lights() const;
	///
	/// \brief Inspect a Node's Transform.
	/// \param out_node Node whose Transform to inspect
	/// \param out_unified_scaling Whether to use a single locked control for scale
	///
	void transform(Node& out_node, Bool& out_unified_scaling) const;
	///
	/// \brief Inspect a Node's instances.
	/// \param out_node Node whose instances to inspect
	/// \param unified_scaling Whether to use a single locked control for scale
	///
	void instances(Node& out_node, Bool unified_scaling) const;
	///
	/// \brief Inspect a Node's Id<Mesh>.
	/// \param out_node Node whose attachment to inspect
	///
	void mesh(Node& out_node) const;
	///
	/// \brief Inspect a Node and its attachments.
	/// \param node_id Id of Node to inspect
	///
	/// Inspects Transform, instances, and Id<Mesh>.
	///
	void node(Id<Node> node_id, Bool& out_unified_scaling) const;
};
} // namespace facade::editor
