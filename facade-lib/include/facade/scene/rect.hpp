#pragma once
#include <glm/vec2.hpp>

namespace facade {
struct Rect {
	float left{};
	float right{};
	float top{};
	float bottom{};

	constexpr glm::vec2 centre() const { return 0.5f * glm::vec2{left + right, top + bottom}; }
	constexpr glm::vec2 top_left() const { return {top, left}; }
	constexpr glm::vec2 top_right() const { return {top, right}; }
	constexpr glm::vec2 bottom_left() const { return {bottom, left}; }
	constexpr glm::vec2 bottom_right() const { return {bottom, right}; }
};
} // namespace facade
