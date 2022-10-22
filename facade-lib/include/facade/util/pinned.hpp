#pragma once

namespace facade {
class Pinned {
  public:
	Pinned() = default;
	Pinned(Pinned&&) = delete;
	Pinned& operator=(Pinned&&) = delete;
	Pinned(Pinned const&) = delete;
	Pinned& operator=(Pinned const&) = delete;
};
} // namespace facade
