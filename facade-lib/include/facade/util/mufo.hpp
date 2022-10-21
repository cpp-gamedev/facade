#pragma once
#include <memory>

namespace facade {
template <typename Type>
	requires(std::is_array_v<Type>)
auto make_unique_for_overwrite(std::size_t element_count) {
#if defined(FACADE_MUFO)
	return std::make_unique_for_overwrite<Type>(element_count);
#else
	return std::make_unique<Type>(element_count);
#endif
}
} // namespace facade
