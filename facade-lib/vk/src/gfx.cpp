#include <facade/vk/gfx.hpp>
#include <limits>

namespace facade {
vk::Result Gfx::wait(vk::ArrayProxy<vk::Fence> const& fences) const { return device.waitForFences(fences, true, std::numeric_limits<std::uint64_t>::max()); }

void Gfx::reset(vk::Fence fence) const {
	wait(fence);
	device.resetFences(fence);
}
} // namespace facade
