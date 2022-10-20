#pragma once
#include <iostream>
#include <syncstream>

namespace facade::logger {
template <typename... Args>
void print_to(std::ostream& out, Args const&... args) {
	auto stream = std::osyncstream{out};
	((stream << args), ...) << '\n';
}

template <typename... Args>
void error(Args const&... args) {
	print_to(std::cerr, "[E] ", args...);
}

template <typename... Args>
void warn(Args const&... args) {
	print_to(std::cout, "[W] ", args...);
}

template <typename... Args>
void info(Args const&... args) {
	print_to(std::cout, "[I] ", args...);
}

template <typename... Args>
void debug(Args const&... args) {
	print_to(std::cout, "[D] ", args...);
}
} // namespace facade::logger
