#pragma once
#include <facade/util/enum_array.hpp>
#include <glm/vec2.hpp>
#include <array>
#include <string_view>

struct GLFWwindow;

namespace facade {
///
/// \brief Kinds of Key / Button actions.
///
enum class Action { eNone, ePress, eHeld, eRepeat, eRelease, eCOUNT_ };
constexpr auto action_str = EnumArray<Action, std::string_view>{
	"none", "press", "held", "repeat", "release",
};

///
/// \brief Storage for Key (or Button) state.
/// \param Capacity size of state array
///
template <int Capacity>
class KeyState {
  public:
	static constexpr int capacity_v{Capacity};

	///
	/// \brief Obtain the action corresponding to key
	/// \param key GLFW key / button Id
	/// \returns Action for key
	///
	Action action(int key) const {
		if (key < 0 || key > capacity_v) { return Action::eNone; }
		return m_key_actions[static_cast<std::size_t>(key)];
	}

	///
	/// \brief Check if a key was pressed.
	/// \param key GLFW key / button Id
	/// \returns true If key was pressed
	///
	bool pressed(int key) const { return action(key) == Action::ePress; }
	///
	/// \brief Check if a key was released.
	/// \param key GLFW key / button Id
	/// \returns true If key was released
	///
	bool released(int key) const { return action(key) == Action::eRelease; }
	///
	/// \brief Check if a key was repeated.
	/// \param key GLFW key / button Id
	/// \returns true If key was repeated
	///
	bool repeated(int key) const { return action(key) == Action::eRepeat; }

	///
	/// \brief Check if a key is held.
	/// \param key GLFW key / button Id
	/// \returns true If key is held
	///
	bool held(int key) const {
		auto const ret = action(key);
		return ret == Action::ePress || ret == Action::eHeld || ret == Action::eRepeat;
	}

	///
	/// \brief Update actions and swap states.
	///
	virtual void next_frame() {
		auto prev = m_key_actions;
		m_key_actions = {};
		for (int key = 0; key < Capacity; ++key) {
			auto const action = prev[static_cast<std::size_t>(key)];
			if (action == Action::ePress || action == Action::eRepeat || action == Action::eHeld) {
				m_key_actions[static_cast<std::size_t>(key)] = Action::eHeld;
			}
		}
	}

	///
	/// \brief Associate action with key.
	/// \param key GLFW key / button Id
	/// \param action Action to associate
	///
	void on_key(int key, Action action) { m_key_actions[static_cast<std::size_t>(key)] = action; }

  private:
	std::array<Action, static_cast<std::size_t>(capacity_v)> m_key_actions{};
};

///
/// \brief Keyboard State
///
class Keyboard : public KeyState<512> {};

///
/// \brief Mouse State
///
class Mouse : public KeyState<8> {
  public:
	///
	/// \brief Wrapper for on_key.
	///
	void on_button(int button, Action action) { on_key(button, action); }
	///
	/// \brief Store cursor position.
	/// \param position Cursor position.
	///
	void on_position(glm::vec2 position) { m_position = position; }
	///
	/// \brief Add scroll delta.
	/// \param delta Amount scrolled in xy directions
	///
	void on_scroll(glm::vec2 delta) { m_scroll += delta; }

	///
	/// \brief Obtain the cursor position.
	/// \returns Cursor position
	///
	glm::vec2 position() const { return m_position; }
	///
	/// \brief Obtain the difference in cursor positions from last state.
	/// \returns Difference in cursor positions.
	///
	glm::vec2 delta_pos() const { return m_position - m_prev_position; }
	///
	/// \brief Obtain the amount scrolled.
	/// \returns Scroll amount
	///
	glm::vec2 scroll() const { return m_scroll; }

	///
	/// \brief Update and swap state.
	///
	void next_frame() override {
		KeyState<8>::next_frame();
		m_scroll = {};
		m_prev_position = m_position;
	}

  private:
	glm::vec2 m_position{};
	glm::vec2 m_prev_position{};
	glm::vec2 m_scroll{};
};

struct Input {
	Keyboard keyboard{};
	Mouse mouse{};
};
} // namespace facade
