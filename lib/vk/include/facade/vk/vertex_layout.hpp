#pragma once
#include <facade/util/fixed_string.hpp>
#include <facade/util/flex_array.hpp>
#include <vulkan/vulkan.hpp>

namespace facade {
struct VertexInput {
	static constexpr std::size_t max_v{16};

	FlexArray<vk::VertexInputAttributeDescription, max_v> attributes{};
	FlexArray<vk::VertexInputBindingDescription, max_v> bindings{};

	std::size_t hash() const;

	bool operator==(VertexInput const& rhs) const { return hash() == rhs.hash(); }
};

struct VertexLayout {
	using ShaderId = FixedString<64>;

	ShaderId shader{};
	VertexInput input{};
};
} // namespace facade
