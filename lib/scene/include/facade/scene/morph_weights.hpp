#pragma once
#include <facade/util/flex_array.hpp>

namespace facade {
struct MorphWeights {
	static constexpr std::size_t max_weights_v{8};

	FlexArray<float, max_weights_v> weights{};
};
} // namespace facade
