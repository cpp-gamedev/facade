#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/scene/id.hpp>
#include <facade/util/fixed_string.hpp>

namespace facade {
class Scene;
class Node;

namespace editor {
///
/// \brief Target Node to inspect (driven by SceneTree)
///
struct InspectNode {
	FixedString<128> name{"[Node]###Node"};
	Id<Node> id{};

	explicit operator bool() const { return id > Id<Node>{}; }
};

///
/// \brief Stateless wrapper to display interactive scene tree and pick Nodes to inspect
///
class SceneTree : public Pinned {
  public:
	SceneTree(Scene& scene) : m_scene(scene) {}

	///
	/// \brief Render the scene tree into the passed window
	/// \param inout Used to highlight matching Node, and set to clicked Node, if any
	/// \returns true if inout changed during tree walk
	///
	bool render(NotClosed<Window>, InspectNode& inout) const;

  private:
	Scene& m_scene;
};
} // namespace editor
} // namespace facade
