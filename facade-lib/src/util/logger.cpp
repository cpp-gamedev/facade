#include <facade/util/logger.hpp>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#endif

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

struct Storage {
	struct Buffer {
		std::size_t limit{};
		std::size_t extra{};

		std::vector<logger::Entry> entries{};
	};

	Buffer buffer{};
	std::mutex mutex{};
};

GetThreadId get_thread_id{};
Storage g_storage{};
} // namespace

int logger::thread_id() { return get_thread_id(); }

std::string logger::format(Level level, std::string_view const message) {
	return fmt::format(fmt::runtime(g_format), fmt::arg("thread", thread_id()), fmt::arg("level", levels_v[static_cast<std::size_t>(level)]),
					   fmt::arg("message", message), fmt::arg("timestamp", make_timestamp()));
}

void logger::log_to(Pipe pipe, Entry entry) {
	auto* fd = pipe == Pipe::eStdErr ? stderr : stdout;
	std::fprintf(fd, "%s\n", entry.message.c_str());
	auto lock = std::scoped_lock{g_storage.mutex};
#if defined(_WIN32)
	OutputDebugStringA(entry.message.c_str());
	OutputDebugStringA("\n");
#endif
	g_storage.buffer.entries.push_back(std::move(entry));
}

void logger::access_buffer(Accessor& accessor) {
	auto lock = std::scoped_lock{g_storage.mutex};
	accessor(g_storage.buffer.entries);
}

logger::Instance::Instance(std::size_t buffer_limit, std::size_t buffer_extra) {
	auto lock = std::scoped_lock{g_storage.mutex};
	g_storage.buffer = Storage::Buffer{buffer_limit, buffer_extra};
}

logger::Instance::~Instance() {
	auto lock = std::scoped_lock{g_storage.mutex};
	g_storage.buffer.entries.clear();
}
} // namespace facade
