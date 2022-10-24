#pragma once
#include <fmt/format.h>
#include <facade/defines.hpp>
#include <facade/util/pinned.hpp>
#include <span>

namespace facade::logger {
enum class Pipe : std::uint8_t { eStdOut, eStdErr };
enum class Level : std::uint8_t { eError, eWarn, eInfo, eDebug, eCOUNT_ };
inline constexpr char levels_v[] = {'E', 'W', 'I', 'D'};

inline std::string g_format{"[T{thread}] [{level}] {message} [{timestamp}]"};

struct Entry {
	std::string message{};
	Level level{};
};

struct Accessor {
	virtual void operator()(std::span<Entry const> entries) = 0;
};

int thread_id();
std::string format(Level level, std::string_view const message);
void log_to(Pipe pipe, Entry entry);
void access_buffer(Accessor& accessor);

class Instance : public Pinned {
  public:
	Instance(std::size_t buffer_limit = 500, std::size_t buffer_extra = 100);
	~Instance();
};

template <typename... Args>
std::string format(Level level, fmt::format_string<Args...> fmt, Args const&... args) {
	return format(level, fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void error(fmt::format_string<Args...> fmt, Args const&... args) {
	log_to(Pipe::eStdErr, {format(Level::eError, fmt, args...), Level::eError});
}

template <typename... Args>
void warn(fmt::format_string<Args...> fmt, Args const&... args) {
	log_to(Pipe::eStdOut, {format(Level::eWarn, fmt, args...), Level::eWarn});
}

template <typename... Args>
void info(fmt::format_string<Args...> fmt, Args const&... args) {
	log_to(Pipe::eStdOut, {format(Level::eInfo, fmt, args...), Level::eInfo});
}

template <typename... Args>
void debug(fmt::format_string<Args...> fmt, Args const&... args) {
	if constexpr (debug_v) { log_to(Pipe::eStdOut, {format(Level::eDebug, fmt, args...), Level::eDebug}); }
}
} // namespace facade::logger
