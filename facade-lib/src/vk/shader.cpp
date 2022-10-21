#include <facade/vk/shader.hpp>

namespace facade {
bool Shader::Db::add(Shader shader) {
	if (shader.id.empty() || !shader.vert || !shader.frag) { return false; }
	auto entry = Entry{.vvert = shader.vert, .vfrag = shader.frag};
	m_map.insert_or_assign(shader.id, std::move(entry));
	return true;
}

Shader Shader::Db::add(std::string_view id, SpirV vert, SpirV frag) {
	if (id.empty() || !vert || !frag) { return {}; }
	auto entry = Entry{std::move(vert), std::move(frag)};
	auto [it, _] = m_map.insert_or_assign(std::move(id), std::move(entry));
	return {it->first, it->second.vert, it->second.frag};
}

Shader Shader::Db::find(std::string_view id) const {
	if (auto it = m_map.find(id); it != m_map.end()) {
		auto vert = it->second.vert ? it->second.vert : it->second.vvert;
		auto frag = it->second.frag ? it->second.frag : it->second.vfrag;
		return {it->first, vert, frag};
	}
	return {};
}
} // namespace facade
