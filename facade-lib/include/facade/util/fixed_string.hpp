#pragma once
#include <cassert>
#include <string_view>

namespace facade {
template <std::size_t Capacity>
class FixedString {
  public:
	template <std::size_t N>
		requires(N <= Capacity)
	consteval FixedString(char const (&str)[N]) : m_size(N) {
		for (std::size_t i = 0; i < m_size; ++i) { m_buf[i] = str[i]; }
	}

	constexpr FixedString(std::string_view const str) : m_size(std::min(str.size(), Capacity)) {
		assert(str.size() <= Capacity);
		for (std::size_t i = 0; i < m_size; ++i) { m_buf[i] = str[i]; }
	}

	constexpr std::string_view view() const { return {m_buf, m_size}; }
	constexpr operator std::string_view() const { return view(); }

	bool operator==(FixedString const&) const = default;

  private:
	char m_buf[Capacity]{};
	std::size_t m_size{};
};
} // namespace facade
