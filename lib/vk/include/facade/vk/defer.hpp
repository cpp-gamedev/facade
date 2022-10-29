#pragma once
#include <facade/util/unique.hpp>
#include <facade/vk/defer_queue.hpp>

namespace facade {
template <typename T>
class Defer {
  public:
	Defer() = default;
	Defer(DeferQueue& queue) : m_t{T{}, Deleter{&queue}} {}
	Defer(T t, DeferQueue& queue) : m_t{std::move(t), Deleter{&queue}} {}

	T& get() { return m_t.get(); }
	T const& get() const { return m_t.get(); }

	void swap(T t) { m_t = {std::move(t), m_t.get_deleter()}; }

  private:
	struct Deleter {
		DeferQueue* queue{};
		void operator()(T t) const {
			if (queue) { queue->push(std::move(t)); }
		}
	};
	Unique<T, Deleter> m_t{};
};
} // namespace facade
