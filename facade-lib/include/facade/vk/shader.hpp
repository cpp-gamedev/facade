#pragma once
#include <facade/util/fixed_string.hpp>
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
	Shader add(std::string_view id, SpirV vert, SpirV frag);
	bool add(Shader shader);
	bool contains(std::string_view id) const { return m_map.contains(id); }
	Shader find(std::string_view id) const;

  private:
	using ShaderId = FixedString<16>;

	struct Entry {
		SpirV vert{};
		SpirV frag{};
		SpirV::View vvert{};
		SpirV::View vfrag{};
	};

	std::unordered_map<ShaderId, Entry, std::hash<std::string_view>> m_map{};
};
} // namespace facade
