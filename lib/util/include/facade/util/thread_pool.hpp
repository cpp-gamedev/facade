#pragma once
#include <facade/util/async_queue.hpp>
#include <facade/util/unique_task.hpp>
#include <future>
#include <thread>
#include <vector>

namespace facade {
///
/// \brief Pool of fixed number of threads and a single shared task queue.
///
class ThreadPool {
  public:
	///
	/// \brief Construct a ThreadPool.
	/// \param threads Number of threads to create
	///
	ThreadPool(std::uint32_t threads = std::thread::hardware_concurrency());
	~ThreadPool();

	///
	/// \brief Schedule a task to be run on the thread pool.
	/// \param func The task to enqueue.
	/// \returns Future of task invocation result
	///
	template <typename F>
	auto enqueue(F func) -> std::future<std::invoke_result_t<F>> {
		using Ret = std::invoke_result_t<F>;
		auto promise = std::promise<Ret>{};
		auto ret = promise.get_future();
		push(std::move(promise), std::move(func));
		return ret;
	}

  private:
	template <typename T, typename F>
	void push(std::promise<T>&& promise, F func) {
		m_queue.push([p = std::move(promise), f = std::move(func)]() mutable {
			try {
				if constexpr (std::is_void_v<T>) {
					f();
					p.set_value();
				} else {
					p.set_value(f());
				}
			} catch (...) {
				try {
					p.set_exception(std::current_exception());
				} catch (...) {}
			}
		});
	}

	AsyncQueue<UniqueTask<void()>> m_queue{};
	std::vector<std::jthread> m_agents{};
};
} // namespace facade
