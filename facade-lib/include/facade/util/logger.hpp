#pragma once
#include <fmt/format.h>
#include <facade/defines.hpp>

namespace facade::logger {
enum class Pipe { eStdOut, eStdErr };

inline std::string g_format{"[{thread}] [{severity}] {message} [{timestamp}]"};

int thread_id();
std::string format(char severity, std::string_view const message);
void print_to(Pipe pipe, char const* text);

template <typename... Args>
std::string format(char severity, fmt::format_string<Args...> fmt, Args const&... args) {
	return format(severity, fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void error(fmt::format_string<Args...> fmt, Args const&... args) {
	print_to(Pipe::eStdErr, format('E', fmt, args...).c_str());
}

template <typename... Args>
void warn(fmt::format_string<Args...> fmt, Args const&... args) {
	print_to(Pipe::eStdOut, format('W', fmt, args...).c_str());
}

template <typename... Args>
void info(fmt::format_string<Args...> fmt, Args const&... args) {
	print_to(Pipe::eStdOut, format('I', fmt, args...).c_str());
}

template <typename... Args>
void debug(fmt::format_string<Args...> fmt, Args const&... args) {
	if constexpr (debug_v) { print_to(Pipe::eStdOut, format('D', fmt, args...).c_str()); }
}
} // namespace facade::logger
