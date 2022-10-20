#pragma once
#include <memory>
#include <span>

namespace facade {
struct ByteBuffer {
	std::unique_ptr<std::byte[]> bytes{};
	std::size_t size{};

	static ByteBuffer make(std::size_t size) { return {std::make_unique_for_overwrite<std::byte[]>(size), size}; }

	std::byte* data() const { return bytes.get(); }
	std::span<std::byte const> span() const { return {data(), size}; }

	explicit operator bool() const { return !!bytes; }
};
} // namespace facade
