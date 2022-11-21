#pragma once
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>

namespace facade {
class Skybox {
  public:
	struct Data;

	Skybox(Gfx const& gfx);

	// +x-x+y-y+z-z
	void set(std::span<Image::View const> images);
	void reset();

	StaticMesh const& static_mesh() const { return m_cube; }
	Cubemap const& cubemap() const { return m_cubemap; }

  private:
	Sampler m_sampler;
	StaticMesh m_cube;
	Cubemap m_cubemap;
	Gfx m_gfx;
};

struct Skybox::Data {
	// +x-x+y-y+z-z
	Image images[6]{};
	mutable Image::View cache[6]{};

	std::span<Image::View const> views() const {
		for (std::size_t i = 0; i < std::size(images); ++i) { cache[i] = images[i]; }
		return cache;
	}
};
} // namespace facade
