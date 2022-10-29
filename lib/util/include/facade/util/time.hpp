#pragma once

namespace facade {
namespace time {
///
/// \brief Obtain time in seconds since app start.
///
float since_start();
} // namespace time

///
/// \brief Stateful delta-time computer
///
struct DeltaTime {
	///
	/// \brief Time of last update.
	///
	float start{};
	///
	/// \brief Cached value.
	///
	float value{};

	///
	/// \brief Update start time and obtain delta time.
	/// \returns Current delta time
	///
	float operator()();
};
} // namespace facade
