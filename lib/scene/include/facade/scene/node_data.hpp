#pragma once
#include <facade/util/transform.hpp>
#include <string>
#include <vector>

namespace facade {
///
/// \brief Scene Node data: shared between Scene and gltf::* (internal)
///
struct NodeData {
	///
	/// \brief Node type.
	///
	enum class Type { eNone, eMesh, eCamera };

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
	/// \brief Type of the node.
	///
	Type type{};
};
} // namespace facade
