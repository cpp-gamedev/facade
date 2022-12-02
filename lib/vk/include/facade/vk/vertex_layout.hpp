#pragma once
#include <facade/util/flex_array.hpp>
#include <vulkan/vulkan.hpp>

namespace facade {
struct VertexLayout {
	static constexpr std::size_t max_v{16};

	FlexArray<vk::VertexInputAttributeDescription, max_v> attributes{};
	FlexArray<vk::VertexInputBindingDescription, max_v> bindings{};

	std::size_t hash() const;

	bool operator==(VertexLayout const& rhs) const { return hash() == rhs.hash(); }
};
} // namespace facade
