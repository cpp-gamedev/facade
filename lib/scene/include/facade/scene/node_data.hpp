#pragma once
#include <facade/util/transform.hpp>
#include <optional>
#include <string>
#include <vector>

namespace facade {
///
/// \brief Scene Node data: shared between Scene and gltf::* (internal)
///
struct NodeData {
	///
	/// \brief Name of the node.
	///
	std::string name{};
	///
	/// \brief Transform for the node.
	///
	Transform transform{};
	///
	/// \brief Indices of child nodes.
	///
	std::vector<std::size_t> children{};
	///
	/// \brief Index of the node.
	///
	std::size_t index{};
	///
	/// \brief Camera index.
	///
	std::optional<std::size_t> camera{};
	///
	/// \brief Mesh index.
	///
	std::optional<std::size_t> mesh{};
	std::optional<std::size_t> skin{};
};
} // namespace facade
