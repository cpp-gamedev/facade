#include <facade/util/error.hpp>
#include <facade/util/thread_pool.hpp>
#include <algorithm>

namespace facade {
ThreadPool::ThreadPool(std::uint32_t const threads) {
	if (threads <= 0) { throw Error{std::string{"Invalid thread count: " + std::to_string(threads)}}; }
	auto agent = [queue = &m_queue](std::stop_token const& stop) {
		while (auto task = queue->pop(stop)) { (*task)(); }
	};
	for (std::uint32_t i = 0; i < threads; ++i) { m_agents.push_back(std::jthread{agent}); }
}

ThreadPool::~ThreadPool() {
	for (auto& thread : m_agents) { thread.request_stop(); }
	m_queue.release();
}
} // namespace facade
