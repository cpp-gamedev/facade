#pragma once
#include <facade/util/fixed_string.hpp>
#include <facade/vk/spir_v.hpp>
#include <unordered_map>

namespace facade {
struct Shader {
	class Db;
	using Id = FixedString<64>;
	struct Program;

	Id id{};
	SpirV::View spir_v{};

	explicit operator bool() const { return static_cast<bool>(spir_v); }
	bool operator==(Shader const& rhs) const { return id == rhs.id; }
};

struct Shader::Program {
	Shader vert{};
	Shader frag{};
};

class Shader::Db {
  public:
	Shader add(Id const& id, SpirV spir_v);
	bool add(Shader shader);
	bool contains(Id const& id) const { return m_map.contains(id); }
	Shader find(Id const& id) const;

  private:
	struct Entry {
		SpirV spir_v{};
		SpirV::View view{};
	};

	std::unordered_map<Id, Entry, std::hash<std::string_view>> m_map{};
};
} // namespace facade
