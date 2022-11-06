#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/engine/editor/log.hpp>
#include <facade/engine/editor/scene_tree.hpp>
#include <string>
#include <vector>

namespace facade {
struct Lights;
class Events;

namespace event {
struct OpenRecent {
	std::string path{};
};
struct OpenFile {};
} // namespace event

class FileMenu {
  public:
	void add_recent(std::string path);
	void display(Events const& events, editor::NotClosed<editor::MainMenu> main, Bool loading);
	std::span<std::string const> recents() const { return m_recents; }

	std::uint8_t max_recents{8};

  private:
	void open_recent(Events const& events, editor::NotClosed<editor::MainMenu> main, Bool loading);

	std::vector<std::string> m_recents;
};

class WindowMenu : public logger::Accessor {
  public:
	void display_menu(editor::NotClosed<editor::MainMenu> main);
	void display_windows(Engine& engine);

  private:
	bool display_scene(Scene& scene);
	bool display_inspector(Scene& scene);
	bool display_lights(Lights& lights);
	bool display_stats(Engine& engine);
	bool display_log();

	void change_vsync(Engine const& engine) const;

	void operator()(std::span<logger::Entry const> entries) final;

	struct {
		editor::LogState log_state{};
		editor::InspectNode inspect{};
		Bool unified_scaling{true};
	} m_data{};
	struct {
		bool scene{};
		bool lights{};
		bool stats{};
		bool log{};
		bool close{};
		bool demo{};
	} m_flags{};
};
} // namespace facade
