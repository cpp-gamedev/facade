#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skybox.hpp>
#include <numeric>

namespace facade {
namespace {
std::array<Image::View, 6> make_blank_cubemap_images() {
	static constexpr std::byte black_v[] = {0x0_B, 0x0_B, 0x0_B, 0xff_B};
	static constexpr auto image_v = Image::View{.bytes = black_v, .extent = {1, 1}};
	return {image_v, image_v, image_v, image_v, image_v, image_v};
}
} // namespace

Skybox::Skybox(Gfx const& gfx)
	: m_sampler(gfx), m_cube(gfx, Geometry::Packed::from(make_cube(glm::vec3{1.0f}))),
	  m_cubemap(gfx, m_sampler.sampler(), make_blank_cubemap_images(), "skybox"), m_gfx(gfx) {}

void Skybox::set(std::span<Image::View const> images) { m_cubemap = {m_gfx, m_sampler.sampler(), images, "skybox"}; }

void Skybox::reset() { m_cubemap = {m_gfx, m_sampler.sampler(), make_blank_cubemap_images()}; }
} // namespace facade
