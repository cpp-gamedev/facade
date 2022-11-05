#pragma once
#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <vector>

namespace facade {
struct Config {
	class Scoped;

	struct {
		glm::uvec2 extent{1280, 720};
		std::optional<glm::ivec2> position{};
		std::uint8_t msaa{};
	} window{};

	struct {
		std::string browse_path{};
		std::vector<std::string> recents{};
	} file_menu{};

	static Config load(char const* path);
	bool save(char const* path) const;
};

class Config::Scoped {
	std::string m_path{};

  public:
	Scoped(std::string path = "facade.conf") : m_path(std::move(path)), config(load(m_path.c_str())) {}
	~Scoped() { config.save(m_path.c_str()); }

	Config config{};
};
} // namespace facade
