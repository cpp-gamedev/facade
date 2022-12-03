#pragma once
#include <facade/scene/animation.hpp>
#include <facade/scene/camera.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/mesh.hpp>
#include <facade/scene/node.hpp>
#include <facade/scene/resource_array.hpp>
#include <facade/scene/skin.hpp>
#include <facade/util/type_id.hpp>
#include <facade/vk/mesh_primitive.hpp>
#include <facade/vk/texture.hpp>

namespace facade {
///
/// \brief Collection of all ResourceArray instances used by Scene.
///
struct SceneResources {
	ResourceArray<Animation> animations{};
	ResourceArray<Camera> cameras{};
	ResourceArray<Sampler> samplers{};
	ResourceArray<Material> materials{};
	ResourceArray<MeshPrimitive> primitives{};
	ResourceArray<Skin> skins{};
	ResourceArray<Texture> textures{};
	ResourceArray<Mesh> meshes{};
	ResourceArray<Node> nodes{};
};
} // namespace facade
