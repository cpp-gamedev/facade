#include <config/config.hpp>
#include <djson/json.hpp>
#include <facade/util/logger.hpp>
#include <filesystem>

namespace facade {
namespace {
template <int Dim, typename T>
bool to_json(dj::Json& out, glm::vec<Dim, T> const& vec) {
	out["x"] = vec.x;
	if constexpr (Dim > 1) { out["y"] = vec.y; }
	if constexpr (Dim > 2) { out["z"] = vec.z; }
	if constexpr (Dim > 3) { out["w"] = vec.w; }
	return true;
}

bool to_json(dj::Json& out, Config const& config) {
	auto window = dj::Json{};
	to_json(window["extent"], config.window.extent);
	if (config.window.position) { to_json(window["position"], *config.window.position); }
	window["msaa"] = static_cast<int>(config.window.msaa);
	auto file_menu = dj::Json{};
	file_menu["browse_path"] = config.file_menu.browse_path;
	if (!config.file_menu.recents.empty()) {
		auto recents = dj::Json{};
		for (auto const& path : config.file_menu.recents) { recents.push_back(path); }
		file_menu["recents"] = std::move(recents);
	}
	out["window"] = std::move(window);
	out["file_menu"] = std::move(file_menu);
	return true;
}

template <int Dim, typename T>
bool from_json(dj::Json const& json, glm::vec<Dim, T>& out) {
	out.x = json["x"].as<T>();
	if constexpr (Dim > 1) { out.y = json["y"].as<T>(); }
	if constexpr (Dim > 2) { out.z = json["z"].as<T>(); }
	if constexpr (Dim > 3) { out.w = json["w"].as<T>(); }
	return true;
}

bool from_json(dj::Json const& json, Config& out) {
	auto const& window = json["window"];
	auto extent = glm::uvec2{};
	from_json<2, std::uint32_t>(window, extent);
	if (extent.x > 0 && extent.y > 0) { out.window.extent = extent; }
	if (auto const& position = window["position"]) {
		auto value = glm::ivec2{};
		from_json<2, std::int32_t>(position, value);
		out.window.position = value;
	}
	if (auto const& msaa = window["msaa"]) { out.window.msaa = static_cast<std::uint8_t>(msaa.as<int>()); }
	auto const& file_menu = json["file_menu"];
	out.file_menu.browse_path = file_menu["browse_path"].as_string();
	for (auto const& path : file_menu["recents"].array_view()) { out.file_menu.recents.emplace_back(path.as_string()); }
	return true;
}
} // namespace

Config Config::load(char const* path) {
	auto ret = Config{};
	if (!std::filesystem::is_regular_file(path)) { return ret; }
	auto json = dj::Json::from_file(path);
	from_json(json, ret);
	logger::info("Loaded config from [{}]", path);
	return ret;
}

bool Config::save(char const* path) const {
	auto json = dj::Json{};
	if (!to_json(json, *this)) { return false; }
	if (!json.to_file(path)) {
		logger::warn("Failed to save config to [{}]", path);
		return false;
	}
	logger::info("Saved config to [{}]", path);
	return true;
}
} // namespace facade
