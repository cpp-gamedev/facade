#pragma once
#include <facade/scene/id.hpp>
#include <optional>

namespace facade {
class MeshPrimitive;
struct Material;

///
/// \brief Primitive topology.
///
enum class Topology {
	ePoints,
	eLines,
	eLineStrip,
	eTriangles,
	eTriangleStrip,
	eTriangleFan,
};

///
/// \brief Represents a Mesh in a Scene.
///
struct Mesh {
	///
	/// \brief Represents a primitive in a mesh
	///
	struct Primitive {
		///
		/// \brief Id of the MeshPrimitive this primitive uses.
		///
		Id<MeshPrimitive> primitive{};
		///
		/// \brief Id of the Material this primitive uses.
		///
		std::optional<Id<Material>> material{};
		///
		/// \brief Primitive topology.
		///
		Topology topology{Topology::eTriangles};
	};

	std::string name{"(Unnamed)"};
	///
	/// \brief List of primitives in this mesh.
	///
	std::vector<Primitive> primitives{};
};
} // namespace facade
