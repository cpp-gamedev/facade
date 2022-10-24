#pragma once
#include <facade/scene/transform.hpp>
#include <string>
#include <vector>

namespace facade {
struct NodeData {
	enum class Type { eNone, eMesh, eCamera };

	std::string name{};
	Transform transform{};
	std::vector<std::size_t> children{};
	std::size_t index{};
	Type type{};
};
} // namespace facade
