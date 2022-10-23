#include <facade/util/logger.hpp>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace facade {
namespace {
struct Timestamp {
	char buffer[32]{};
	operator char const*() const { return buffer; }
};

Timestamp make_timestamp() {
	auto const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto ret = Timestamp{};
	if (!std::strftime(ret.buffer, sizeof(ret.buffer), "%H:%M:%S", std::localtime(&now))) { return {}; }
	return ret;
}

struct GetThreadId {
	std::unordered_map<std::thread::id, int> ids{};
	int next{};
	std::mutex mutex{};

	int operator()() {
		auto lock = std::scoped_lock{mutex};
		auto const tid = std::this_thread::get_id();
		if (auto const it = ids.find(tid); it != ids.end()) { return it->second; }
		auto const [it, _] = ids.insert_or_assign(tid, next++);
		return it->second;
	}
};

GetThreadId get_thread_id{};
} // namespace

int logger::thread_id() { return get_thread_id(); }

std::string logger::format(char severity, std::string_view const message) {
	auto const ts = fmt::arg("timestamp", make_timestamp());
	return fmt::format(fmt::runtime(g_format), fmt::arg("thread", thread_id()), fmt::arg("severity", severity), fmt::arg("message", message), ts);
}

void logger::print_to(Pipe pipe, char const* text) {
	auto* fd = pipe == Pipe::eStdErr ? stderr : stdout;
	std::fprintf(fd, "%s\n", text);
}
} // namespace facade
