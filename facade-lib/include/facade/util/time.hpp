#pragma once

namespace facade {
namespace time {
float since_start();
} // namespace time

struct DeltaTime {
	float start{};

	float operator()();
};
} // namespace facade
