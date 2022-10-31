#pragma once
#include <array>
#include <cassert>
#include <span>

namespace facade {
///
/// \brief Concept for types that can be stored in a FlexArray.
///
template <typename Type>
concept FlexArrayItem = std::is_default_constructible_v<Type> && std::is_move_constructible_v<Type>;

///
/// \brief Lightweight "flexible" length stack array.
///
template <FlexArrayItem Type, std::size_t Capacity>
class FlexArray {
  public:
	///
	/// \brief Insert an element into the array and increment the size.
	/// \param t Element to move and insert
	/// \returns Reference to itself
	///
	/// Note: current size must be less than Capacity!
	///
	constexpr Type& insert(Type t) {
		assert(m_size < Capacity);
		auto& ret = m_t[m_size++];
		ret = std::move(t);
		return ret;
	}

	///
	/// \brief Clear the array and size.
	///
	constexpr void clear() {
		m_t = {};
		m_size = {};
	}

	///
	/// \brief Obtain a mutable view into the array.
	///
	constexpr std::span<Type> span() { return {m_t.data(), m_size}; }
	///
	/// \brief Obtain an immutable view into the array.
	///
	constexpr std::span<Type const> span() const { return {m_t.data(), m_size}; }

	///
	/// \brief Obtain the number of elements stored.
	///
	constexpr std::size_t size() const { return m_size; }

  private:
	std::array<Type, Capacity> m_t{};
	std::size_t m_size{};
};
} // namespace facade
