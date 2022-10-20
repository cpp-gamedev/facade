#pragma once
#include <stdexcept>

namespace facade {
struct Error : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InitError : Error {
	using Error::Error;
};
struct ValidationError : Error {
	using Error::Error;
};
} // namespace facade
