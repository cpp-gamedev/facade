#pragma once
#include <facade/util/bool.hpp>
#include <facade/util/pinned.hpp>
#include <glm/vec2.hpp>
#include <cassert>
#include <concepts>

namespace facade::editor {
///
/// \brief Base class for RAII Dear ImGui wrappers whose widgets return a boolean on Begin()
///
class Openable : public Pinned {
  public:
	bool is_open() const { return m_open; }
	explicit operator bool() const { return is_open(); }

  protected:
	Openable(bool is_open = false);
	bool m_open;
};

///
/// \brief Required dependency on an Openable which must be open (when used as an argument)
///
template <typename T>
struct NotClosed {
	NotClosed([[maybe_unused]] T const& t) { assert(t.is_open()); }
};

///
/// \brief RAII Dear ImGui window
///
class Window : public Openable {
  public:
	class Menu;

	explicit Window(char const* label, bool* open_if = {}, int flags = {});
	Window(NotClosed<Window> parent, char const* label, glm::vec2 size = {}, Bool border = {}, int flags = {});
	~Window();

  private:
	bool m_child{};
};

///
/// \brief RAII Dear ImGui TreeNode
///
class TreeNode : public Openable {
  public:
	explicit TreeNode(char const* label, int flags = {});
	~TreeNode();
};

///
/// \brief Base class for Dear ImGui MenuBars
///
class MenuBar : public Openable {
  protected:
	using Openable::Openable;
};

///
/// \brief RAII Dear ImGui Menu object
///
class Menu : public Openable {
  public:
	explicit Menu(NotClosed<MenuBar>, char const* label, Bool enabled = {true});
	~Menu();
};

///
/// \brief RAII Dear ImGui windowed MenuBar
///
class Window::Menu : public MenuBar {
  public:
	explicit Menu(NotClosed<Window>);
	~Menu();
};

///
/// \brief RAII Dear ImGui MainMenuBar
///
class MainMenu : public MenuBar {
  public:
	explicit MainMenu();
	~MainMenu();
};

///
/// \brief RAII Dear ImGui Popup
///
class Popup : public Openable {
  public:
	explicit Popup(char const* id, Bool centered = {}, int flags = {}) : Popup(id, {}, centered, flags) {}
	~Popup();

	static void open(char const* id);
	static void close_current();

  protected:
	explicit Popup(char const* id, Bool modal, Bool centered, int flags);
};

///
/// \brief RAII Dear ImGui PopupModal
///
class Modal : public Popup {
  public:
	explicit Modal(char const* id, Bool centered = {true}, int flags = {}) : Popup(id, {true}, centered, flags) {}
};

///
/// \brief RAII Dear ImGui StyleVar stack
///
class StyleVar : public Pinned {
  public:
	explicit StyleVar(int index, glm::vec2 value);
	explicit StyleVar(int index, float value);
	~StyleVar();

	explicit operator bool() const { return true; }
};
} // namespace facade::editor
