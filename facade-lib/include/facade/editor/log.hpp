#pragma once
#include <facade/util/enum_array.hpp>
#include <facade/util/logger.hpp>
#include <vector>

namespace facade::editor {
class Log : public logger::Accessor {
  public:
	void render();

	bool show{};

  private:
	void operator()(std::span<logger::Entry const> entries) final;

	std::vector<logger::Entry const*> m_list{};
	EnumArray<logger::Level, bool> m_level_filter{true, true, true, debug_v};
	int m_display_count{50};
};
} // namespace facade::editor
