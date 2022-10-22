#pragma once
#include <facade/util/pinned.hpp>

namespace facade::editor {
class Window : public Pinned {
  public:
	explicit Window(char const* label, bool* open_if = {}, int flags = {});
	~Window();

	bool is_open() const { return m_open; }
	explicit operator bool() const { return is_open(); }

  private:
	bool m_open{};
};

class TreeNode : public Pinned {
  public:
	explicit TreeNode(char const* label, int flags = {});
	~TreeNode();

	bool is_open() const { return m_open; }
	explicit operator bool() const { return is_open(); }

  private:
	bool m_open{};
};
} // namespace facade::editor
