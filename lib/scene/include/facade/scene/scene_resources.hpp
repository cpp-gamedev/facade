#pragma once
#include <facade/scene/camera.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/mesh.hpp>
#include <facade/util/ptr.hpp>
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>
#include <span>

namespace facade {
template <typename T>
class ResourceArray {
  public:
	std::span<T> view() { return m_array; }
	std::span<T const> view() const { return m_array; }

	Ptr<T const> find(Id<T> index) const {
		if (index >= m_array.size()) { return {}; }
		return &m_array[index];
	}

	Ptr<T> find(Id<T> index) { return const_cast<Ptr<T>>(std::as_const(*this).find(index)); }

	T const& operator[](Id<T> index) const {
		auto ret = find(index);
		assert(ret);
		return *ret;
	}

	T& operator[](Id<T> index) { return const_cast<T&>(std::as_const(*this).operator[](index)); }

	constexpr std::size_t size() const { return m_array.size(); }
	constexpr bool empty() const { return m_array.empty(); }

  private:
	std::vector<T> m_array{};

	friend class Scene;
};

struct SceneResources {
	ResourceArray<Camera> cameras{};
	ResourceArray<Sampler> samplers{};
	ResourceArray<Material> materials{};
	ResourceArray<StaticMesh> static_meshes{};
	ResourceArray<Texture> textures{};
	ResourceArray<Mesh> meshes{};
};
} // namespace facade
