#pragma once
#include <concepts>
#include <cstdint>

namespace facade {
///
/// \brief Represents a strongly-typed integral ID.
///
/// Primarily used for items owned by a Scene.
///
template <typename Type, std::integral Value = std::size_t>
class Id {
  public:
	using id_type = Value;

	///
	/// \brief Implicit constructor
	/// \param value The underlying value of this instance
	///
	constexpr Id(Value value = {}) : m_value(value) {}

	constexpr Value value() const { return m_value; }
	constexpr operator Value() const { return value(); }

	bool operator==(Id const&) const = default;

  private:
	Value m_value{};
};
} // namespace facade
