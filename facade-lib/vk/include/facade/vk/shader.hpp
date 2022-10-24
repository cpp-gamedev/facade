#pragma once
#include <facade/vk/spir_v.hpp>
#include <string>
#include <unordered_map>

namespace facade {
struct Shader {
	class Db;

	std::string_view id{};
	SpirV::View vert{};
	SpirV::View frag{};

	explicit operator bool() const { return vert && frag; }
	bool operator==(Shader const& rhs) const { return id == rhs.id; }
};

class Shader::Db {
  public:
	Shader add(std::string id, SpirV vert, SpirV frag);
	bool add(Shader shader);
	bool contains(std::string const& id) const { return m_map.contains(id); }
	Shader find(std::string const& id) const;

  private:
	struct Entry {
		SpirV vert{};
		SpirV frag{};
		SpirV::View vvert{};
		SpirV::View vfrag{};
	};

	std::unordered_map<std::string, Entry, std::hash<std::string_view>> m_map{};
};
} // namespace facade
