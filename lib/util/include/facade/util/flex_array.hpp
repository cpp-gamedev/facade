#pragma once
#include <array>
#include <cassert>
#include <span>

namespace facade {
template <typename Type, std::size_t Capacity>
class FlexArray {
  public:
	constexpr Type& insert(Type t) {
		assert(m_size < Capacity);
		auto& ret = m_t[m_size++];
		ret = std::move(t);
		return ret;
	}

	constexpr void clear() {
		m_t = {};
		m_size = {};
	}

	constexpr std::span<Type> span() { return {m_t.data(), m_size}; }
	constexpr std::span<Type const> span() const { return {m_t.data(), m_size}; }

	constexpr std::size_t size() const { return m_size; }

  private:
	std::array<Type, Capacity> m_t{};
	std::size_t m_size{};
};
} // namespace facade
