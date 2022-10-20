#pragma once
#include <facade/util/unique.hpp>
#include <glm/vec2.hpp>
#include <span>
#include <string>

namespace facade {
class Image {
  public:
	struct View {
		std::span<std::byte const> bytes{};
		glm::uvec2 extent{};
	};

	Image() = default;

	explicit Image(std::span<std::byte const> compressed, std::string name = {});

	View view() const;
	std::string_view name() const { return m_name; }

	operator View() const { return view(); }
	explicit operator bool() const;

  private:
	struct Storage {
		std::size_t size{};
		std::byte const* data{};

		std::span<std::byte const> bytes() const { return {data, size}; }

		struct Deleter {
			void operator()(Storage const& storage) const;
		};
	};

	std::string m_name{};
	Unique<Storage, Storage::Deleter> m_storage{};
	glm::uvec2 m_extent{};
};
} // namespace facade
