#pragma once
#include <fmt/format.h>

namespace facade {
///
/// \brief Simple "stringy" wrapper around a stack buffer of chars
///
template <std::size_t Capacity = 64>
class FixedString {
  public:
	FixedString() = default;

	template <std::convertible_to<std::string_view> T>
	FixedString(T const& t) {
		auto const str = std::string_view{t};
		m_size = str.size();
		std::memcpy(m_buffer, str.data(), m_size);
	}

	template <typename... Args>
	explicit FixedString(fmt::format_string<Args...> fmt, Args const&... args) {
		auto const [it, _] = fmt::vformat_to_n(m_buffer, Capacity, fmt, fmt::make_format_args(args...));
		m_size = static_cast<std::size_t>(std::distance(m_buffer, it));
	}

	template <std::size_t N>
	void append(FixedString<N> const& rhs) {
		auto const dsize = std::min(Capacity - m_size, rhs.m_size);
		std::memcpy(m_buffer + m_size, rhs.m_buffer, dsize);
		m_size += dsize;
	}

	std::string_view view() const { return {m_buffer, m_size}; }
	char const* data() const { return m_buffer; }
	char const* c_str() const { return m_buffer; }
	std::size_t size() const { return m_size; }
	bool empty() const { return m_size == 0; }

	operator std::string_view() const { return view(); }

	template <std::size_t N>
	FixedString& operator+=(FixedString<N> const& rhs) {
		append(rhs);
		return *this;
	}

  private:
	char m_buffer[Capacity + 1]{};
	std::size_t m_size{};
};
} // namespace facade
