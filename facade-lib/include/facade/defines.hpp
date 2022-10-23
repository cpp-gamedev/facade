#pragma once

namespace facade {
constexpr bool debug_v =
#if defined(FACADE_DEBUG)
	true;
#else
	false;
#endif
} // namespace facade
