#include <facade/util/time.hpp>
#include <chrono>

namespace facade {
namespace {
using Clock = std::chrono::steady_clock;
using Secs = std::chrono::duration<float>;
auto const g_start{Clock::now()};
} // namespace

float time::since_start() { return Secs{Clock::now() - g_start}.count(); }

float DeltaTime::operator()() {
	auto const now = time::since_start();
	value = now - start;
	start = now;
	return value;
}
} // namespace facade
