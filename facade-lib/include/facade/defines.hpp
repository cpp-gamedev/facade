#pragma once

namespace facade {
constexpr bool debug_v =
#if defined(UDUN_DEBUG)
	true;
#else
	false;
#endif
} // namespace facade
