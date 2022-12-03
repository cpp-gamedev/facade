#include <facade/util/hash_combine.hpp>
#include <facade/vk/vertex_layout.hpp>

namespace facade {
std::size_t VertexInput::hash() const {
	auto ret = std::size_t{};
	for (auto const& binding : bindings.span()) { hash_combine(ret, binding.binding, binding.stride, binding.inputRate); }
	for (auto const& attribute : attributes.span()) { hash_combine(ret, attribute.binding, attribute.format, attribute.location, attribute.offset); }
	return ret;
}
} // namespace facade
