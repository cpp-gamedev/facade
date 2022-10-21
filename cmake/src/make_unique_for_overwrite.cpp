#include <memory>

namespace {
[[maybe_unused]] auto make(std::size_t n) { return std::make_unique_for_overwrite<int[]>(n); }
} // namespace
