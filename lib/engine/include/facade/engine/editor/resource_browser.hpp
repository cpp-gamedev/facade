#pragma once
#include <facade/engine/editor/common.hpp>
#include <glm/vec3.hpp>

namespace facade {
class Scene;

namespace editor {
///
/// \brief Stateful browser for resources in a scene.
///
/// Uses a framed TreeNode.
///
class ResourceBrowser {
  public:
	///
	/// \brief View/edit all resources.
	/// \param target editor Window to draw to
	/// \param out_scene Scene whose resources to view/edit
	///
	/// Uses ResourceInspector.
	///
	void display(NotClosed<Window> target, Scene& out_scene);

  private:
	void add_mesh(bool trigger);
	void add_material(bool trigger);
	void add_static_mesh(bool trigger);

	enum class SmType { eCube, eSphere, eCone, eCylinder, eArrow };

	struct {
		struct {
			std::string buffer{};
			int type{};
		} mat{};
		struct {
			std::string buffer{};
		} mesh{};
		struct {
			SmType type{};
			std::string buffer{};
			struct {
				glm::vec3 vertex_rgb{1.0f};
				glm::vec3 size{1.0f};
			} cube{};
			struct {
				glm::vec3 vertex_rgb{1.0f};
				float diameter{1.0f};
				int quads_per_side{32};
			} sphere{};
			struct {
				glm::vec3 vertex_rgb{1.0f};
				float xz_diam{1.0f};
				int xz_points{128};
				float y_height{1.0f};
			} conic{};
			struct {
				glm::vec3 vertex_rgb{1.0f};
				float stalk_diam{0.5f};
				float stalk_height{1.0f};
				int xz_points{128};
			} arrow{};
		} static_mesh{};
	} m_data{};

	Scene* m_scene{};
	NotClosed<Window>* m_target{};
};
} // namespace editor
} // namespace facade
