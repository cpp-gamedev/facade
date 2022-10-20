#pragma once
#include <glm/vec2.hpp>
#include <array>
#include <string_view>

struct GLFWwindow;

namespace facade {
enum class Action { eNone, ePress, eHeld, eRepeat, eRelease };

inline constexpr std::string_view action_name(Action action) {
	switch (action) {
	default:
	case Action::eNone: return "none";
	case Action::ePress: return "press";
	case Action::eHeld: return "held";
	case Action::eRepeat: return "repeat";
	case Action::eRelease: return "release";
	}
}

template <int Capacity>
class KeyState {
  public:
	static constexpr int capacity_v{Capacity};

	Action action(int key) const {
		if (key < 0 || key > capacity_v) { return Action::eNone; }
		return m_key_actions[static_cast<std::size_t>(key)];
	}

	bool pressed(int key) const { return action(key) == Action::ePress; }
	bool released(int key) const { return action(key) == Action::eRelease; }
	bool repeated(int key) const { return action(key) == Action::eRepeat; }

	bool held(int key) const {
		auto const ret = action(key);
		return ret == Action::ePress || ret == Action::eHeld || ret == Action::eRepeat;
	}

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

	void on_key(int key, Action action) { m_key_actions[static_cast<std::size_t>(key)] = action; }

  private:
	std::array<Action, static_cast<std::size_t>(capacity_v)> m_key_actions{};
};

class Keyboard : public KeyState<512> {};

class Mouse : public KeyState<8> {
  public:
	void on_button(int button, Action action) { on_key(button, action); }
	void on_position(glm::vec2 position) { m_position = position; }
	void on_scroll(glm::vec2 scroll) { m_scroll += scroll; }

	glm::vec2 position() const { return m_position; }
	glm::vec2 delta_pos() const { return m_position - m_prev_position; }
	glm::vec2 scroll() const { return m_scroll; }

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
