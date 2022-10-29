#pragma once
#include <stdexcept>

namespace facade {
///
/// \brief Base facade exception.
///
struct Error : std::runtime_error {
	using std::runtime_error::runtime_error;
};

///
/// \brief Error during initialization.
///
struct InitError : Error {
	using Error::Error;
};

///
/// \brief Vulkan Validation error.
///
struct ValidationError : Error {
	using Error::Error;
};
} // namespace facade
