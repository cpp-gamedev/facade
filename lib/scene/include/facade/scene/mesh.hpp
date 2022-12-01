#pragma once
#include <facade/scene/id.hpp>
#include <optional>

namespace facade {
class StaticMesh;
class SkinnedMesh;
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
		/// \brief Id of the StaticMesh this primitive uses.
		///
		Id<StaticMesh> static_mesh{};

		std::optional<Id<SkinnedMesh>> skinned_mesh{};
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
