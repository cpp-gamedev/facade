#pragma once
#include <atomic>
#include <cstdint>

namespace facade {
class TypeId {
  public:
	template <typename Type>
	static TypeId make() {
		return get_id<Type>();
	}

	TypeId() = default;

	std::size_t value() const { return m_id; }
	explicit operator std::size_t() const { return value(); }
	bool operator==(TypeId const&) const = default;

  private:
	TypeId(std::size_t id) : m_id(id) {}

	template <typename T>
	static std::size_t get_id() {
		static auto const ret{++s_next_id};
		return ret;
	}

	inline static std::atomic<std::size_t> s_next_id{};
	std::size_t m_id{};
};
} // namespace facade
