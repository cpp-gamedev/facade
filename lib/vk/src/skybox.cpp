#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skybox.hpp>
#include <numeric>

namespace facade {
namespace {
std::array<Image::View const, 6> make_cubemap_images(Skybox::Images const& images) {
	return {images.right, images.left, images.top, images.bottom, images.front, images.back};
}
} // namespace

Skybox::Skybox(Gfx const& gfx, Images const& images)
	: m_sampler(gfx, Sampler::CreateInfo{.min = vk::Filter::eNearest, .mag = vk::Filter::eNearest}), m_cube(gfx, make_cube(glm::vec3{1.0f})),
	  m_cubemap(gfx, m_sampler.sampler(), make_cubemap_images(images), "skybox"), m_gfx(gfx) {}

void Skybox::set_cubemap(Images const& images) { m_cubemap = {m_gfx, m_sampler.sampler(), make_cubemap_images(images), "skybox"}; }
} // namespace facade
