#pragma once
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>

namespace facade {
class Skybox {
  public:
	struct Images;

	Skybox(Gfx const& gfx, Images const& images);

	void set_cubemap(Images const& images);

	StaticMesh const& static_mesh() const { return m_cube; }
	Cubemap const& cubemap() const { return m_cubemap; }

  private:
	Sampler m_sampler;
	StaticMesh m_cube;
	Cubemap m_cubemap;
	Gfx m_gfx;
};

struct Skybox::Images {
	Image::View right{};
	Image::View left{};
	Image::View top{};
	Image::View bottom{};
	Image::View front{};
	Image::View back{};
};
} // namespace facade
