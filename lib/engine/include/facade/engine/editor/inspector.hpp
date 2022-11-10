#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/node.hpp>
#include <facade/util/nvec3.hpp>
#include <facade/util/rgb.hpp>
#include <limits>

namespace facade {
struct Camera;
struct Mesh;
struct Lights;
struct SceneResources;
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
	///
	Inspector(NotClosed<Window>) {}

	bool inspect(char const* label, glm::vec2& out_vec2, float speed = 1.0f, float lo = min_v, float hi = max_v) const;
	bool inspect(char const* label, glm::vec3& out_vec3, float speed = 1.0f, float lo = min_v, float hi = max_v) const;
	bool inspect(char const* label, glm::vec4& out_vec4, float speed = 1.0f, float lo = min_v, float hi = max_v) const;
	bool inspect(char const* label, nvec3& out_vec3, float speed = 0.01f) const;
	bool inspect(char const* label, glm::quat& out_quat) const;
	bool inspect_rgb(char const* label, glm::vec3& out_rgb) const;
	bool inspect(char const* label, Rgb& out_rgb) const;
	bool inspect(Transform& out_transform, Bool& out_unified_scaling) const;
	bool inspect(std::span<Transform> out_instances, Bool unfied_scaling) const;
	bool inspect(Lights& out_lights) const;

  private:
	bool do_inspect(Transform& out_transform, Bool& out_unified_scaling, Bool scaling_toggle) const;
};

class SceneInspector : public Inspector {
  public:
	using Inspector::inspect;

	SceneInspector(NotClosed<Window> target, Scene& scene);

	bool inspect(NotClosed<TreeNode>, UnlitMaterial& out_material) const;
	bool inspect(NotClosed<TreeNode> node, LitMaterial& out_material) const;
	bool inspect(Id<Material> material_id) const;
	bool inspect(Id<Mesh> mesh) const;

	bool inspect(Id<Node> node_id, Bool& out_unified_scaling) const;

  private:
	Scene& m_scene;
	NotClosed<Window> m_target;
};

class ResourceInspector {
  public:
	ResourceInspector(NotClosed<Window>, SceneResources const& resources);

	void display() const;
	void display(Camera const& camera, std::size_t index, std::string_view prefix = {}) const;
	void display(Texture const& texture, std::size_t index, std::string_view prefix = {}) const;
	void display(Material const& material, std::size_t const index, std::string_view prefix = {}) const;
	void display(Mesh const& mesh, std::size_t const index, std::string_view prefix = {}) const;

  private:
	void display(LitMaterial const& lit) const;
	void display(UnlitMaterial const& unlit) const;

	SceneResources const& m_resources;
};
} // namespace editor
} // namespace facade
