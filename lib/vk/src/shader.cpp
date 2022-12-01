#include <facade/vk/shader.hpp>

namespace facade {
bool Shader::Db::add(Shader shader) {
	if (shader.id.empty() || !shader.spir_v) { return false; }
	auto entry = Entry{.view = shader.spir_v};
	m_map.insert_or_assign(std::string{shader.id}, std::move(entry));
	return true;
}

Shader Shader::Db::add(Id const& id, SpirV spir_v) {
	if (id.empty() || !spir_v) { return {}; }
	auto entry = Entry{std::move(spir_v)};
	auto [it, _] = m_map.insert_or_assign(id, std::move(entry));
	return {it->first, it->second.spir_v};
}

Shader Shader::Db::find(Id const& id) const {
	if (auto it = m_map.find(id); it != m_map.end()) {
		auto view = it->second.spir_v ? it->second.spir_v : it->second.view;
		return {it->first, view};
	}
	return {};
}
} // namespace facade
