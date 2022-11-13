#pragma once
#include <facade/scene/camera.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/mesh.hpp>
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>
#include <span>

namespace facade {
///
/// \brief View of the resources stored in the scene.
///
template <bool Const>
struct TSceneResources {
	template <typename T>
	using View = std::conditional_t<Const, std::span<T const>, std::span<T>>;

	View<Camera> cameras{};
	View<Sampler> samplers{};
	View<std::unique_ptr<Material>> materials{};
	View<StaticMesh> static_meshes{};
	View<Texture> textures{};
	View<Mesh> meshes{};

	operator TSceneResources<true>() const { return {cameras, samplers, materials, static_meshes, textures, meshes}; }
};

using SceneResources = TSceneResources<true>;
using SceneResourcesMut = TSceneResources<false>;
} // namespace facade
