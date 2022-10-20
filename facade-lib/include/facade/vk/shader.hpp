#pragma once
#include <facade/vk/spir_v.hpp>
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
	bool contains(std::string_view id) const { return m_map.contains(id); }
	Shader find(std::string_view id) const;

  private:
	struct Hasher {
		using is_transparent = void;
		size_t operator()(std::string_view const txt) const { return std::hash<std::string_view>{}(txt); }
	};

	struct Entry {
		SpirV vert{};
		SpirV frag{};
		SpirV::View vvert{};
		SpirV::View vfrag{};
	};

	std::unordered_map<std::string, Entry, Hasher, std::equal_to<>> m_map{};
};
} // namespace facade
