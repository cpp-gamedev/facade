#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/engine/editor/log.hpp>
#include <facade/engine/editor/scene_tree.hpp>
#include <string>
#include <variant>
#include <vector>

namespace facade {
struct Lights;

class FileMenu {
  public:
	struct OpenRecent {
		std::string path{};
	};
	struct Shutdown {};
	using Command = std::variant<std::monostate, OpenRecent, Shutdown>;

	void add_recent(std::string path);
	Command display(editor::NotClosed<editor::MainMenu> main);

	std::uint8_t max_recents{8};

  private:
	Command open_recent(editor::NotClosed<editor::MainMenu> main);

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
