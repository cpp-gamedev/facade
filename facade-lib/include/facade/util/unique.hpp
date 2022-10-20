#pragma once
#include <concepts>
#include <utility>

namespace facade {
template <typename Type>
struct DefaultDeleter {
	constexpr void operator()(Type) const {}
};

template <typename Type, typename Deleter = DefaultDeleter<Type>>
class Unique {
  public:
	constexpr Unique(Type t = {}, Deleter deleter = {}) : m_t{std::move(t)}, m_deleter{std::move(deleter)} {}

	constexpr Unique(Unique&& rhs) noexcept : Unique() { swap(rhs); }
	constexpr Unique& operator=(Unique rhs) noexcept { return (swap(rhs), *this); }
	constexpr ~Unique() noexcept { m_deleter(std::move(m_t)); }

	constexpr void swap(Unique& rhs) noexcept {
		using std::swap;
		swap(m_t, rhs.m_t);
		swap(m_deleter, rhs.m_deleter);
	}

	constexpr Type& get() { return m_t; }
	constexpr Type const& get() const { return m_t; }

	explicit constexpr operator bool() const requires(std::equality_comparable<Type>) { return m_t != Type{}; }

	constexpr operator Type&() { return get(); }
	constexpr operator Type const&() const { return get(); }

	constexpr Type& operator->() { return get(); }
	constexpr Type const& operator->() const { return get(); }

	constexpr Deleter& get_deleter() { return m_deleter; }
	constexpr Deleter const& get_deleter() const { return m_deleter; }

  private:
	Type m_t{};
	[[no_unique_address]] Deleter m_deleter{};
};

template <typename Type>
concept Pointer = std::is_pointer_v<Type>;

template <Pointer Type>
struct HeapDeleter {
	void operator()(Type type) const { delete type; }
};

template <typename Type>
using UniquePtr = Unique<Type*, HeapDeleter<Type*>>;

template <typename Type, typename... Args>
auto make_unique_ptr(Args&&... args) {
	return UniquePtr<Type>(new Type(std::forward<Args>(args)...));
}
} // namespace facade
