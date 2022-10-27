#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/node.hpp>
#include <limits>

namespace facade {
struct Mesh;
class Scene;

namespace editor {
///
/// \brief Stateless ImGui helper to inspect / reflect various properties
///
class Inspector {
  public:
	static constexpr auto min_v{-std::numeric_limits<float>::max()};
	static constexpr auto max_v{std::numeric_limits<float>::max()};

	///
	/// \brief Construct an Inspector instance
	///
	/// Inspectors don't do anything on construction, constructors exist to enforce invariants instance-wide.
	/// For all Inspectors, an existing Window target is required, Inspector instances will not create any
	/// Note: target must be open
	///
	Inspector(Window const& target);

	bool inspect(char const* label, glm::vec2& out_vec2, float speed = 1.0f, float lo = min_v, float hi = max_v) const;
	bool inspect(char const* label, glm::vec3& out_vec3, float speed = 1.0f, float lo = min_v, float hi = max_v) const;
	bool inspect(char const* label, glm::vec4& out_vec4, float speed = 1.0f, float lo = min_v, float hi = max_v) const;
	bool inspect(char const* label, glm::quat& out_quat) const;
	bool inspect(Transform& out_transform) const;
};

class SceneInspector : public Inspector {
  public:
	using Inspector::inspect;

	SceneInspector(Window const& target, Scene& scene);

	bool inspect(TreeNode const& node, UnlitMaterial& out_material) const;
	bool inspect(TreeNode const& node, LitMaterial& out_material) const;
	bool inspect(Id<Material> material_id) const;
	bool inspect(Id<Node> node_id) const;

	bool inspect(Id<Mesh> mesh) const;

  private:
	Scene& m_scene;
};
} // namespace editor
} // namespace facade
