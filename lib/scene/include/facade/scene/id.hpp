#pragma once
#include <cstdint>

namespace facade {
template <typename T>
class Id {
  public:
	using id_type = std::size_t;

	constexpr Id(std::size_t value = {}) : m_value(value) {}

	constexpr std::size_t value() const { return m_value; }
	constexpr operator std::size_t() const { return value(); }

	bool operator==(Id const&) const = default;

  private:
	std::size_t m_value{};
};
} // namespace facade
