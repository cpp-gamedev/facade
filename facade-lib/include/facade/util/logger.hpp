#pragma once
#include <facade/util/string.hpp>

namespace facade::logger {
enum class Pipe { eStdOut, eStdErr };

int thread_id();
std::string format(char severity, std::string_view const message);
void print_to(Pipe pipe, char const* text);

template <typename... Args>
void print_formatted(Pipe pipe, char severity, Args const&... args) {
	print_to(pipe, format(severity, concat(args...)).c_str());
}

template <typename... Args>
void error(Args const&... args) {
	print_formatted(Pipe::eStdErr, 'E', args...);
}

template <typename... Args>
void warn(Args const&... args) {
	print_formatted(Pipe::eStdOut, 'W', args...);
}

template <typename... Args>
void info(Args const&... args) {
	print_formatted(Pipe::eStdOut, 'I', args...);
}

template <typename... Args>
void debug(Args const&... args) {
	print_formatted(Pipe::eStdOut, 'D', args...);
}
} // namespace facade::logger
