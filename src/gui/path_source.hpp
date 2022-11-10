#pragma once
#include <string>

namespace facade {
class PathSource {
  public:
	struct List;

	virtual ~PathSource() = default;

	virtual std::string update() = 0;
};

struct PathSource::List {
	std::vector<std::unique_ptr<PathSource>> sources{};

	std::string update() {
		auto ret = std::string{};
		for (auto const& source : sources) {
			auto str = source->update();
			if (ret.empty()) { ret = std::move(str); }
		}
		return ret;
	}
};
} // namespace facade
