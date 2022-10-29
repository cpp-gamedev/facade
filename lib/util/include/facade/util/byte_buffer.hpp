#pragma once
#include <facade/util/mufo.hpp>
#include <span>

namespace facade {
///
/// \brief Stores a buffer of bytes (on the heap).
///
struct ByteBuffer {
	///
	/// \brief The stored data.
	///
	std::unique_ptr<std::byte[]> bytes{};
	///
	/// \brief Number of bytes.
	///
	std::size_t size{};

	///
	/// \brief Create a ByteBuffer of a desired size.
	/// \param size The desired size in bytes
	/// \returns ByteBuffer instance
	///
	static ByteBuffer make(std::size_t size) { return {make_unique_for_overwrite<std::byte[]>(size), size}; }

	///
	/// \brief Obtain a pointer to the start of the stored data.
	/// \returns nullptr if nothing is stored
	///
	std::byte* data() const { return bytes.get(); }
	///
	/// \brief Obtain a read-only span into the stored data.
	/// \returns std::span into data
	///
	std::span<std::byte const> span() const { return {data(), size}; }

	///
	/// \brief Check if any data is stored.
	/// \returns true If data is present
	///
	explicit operator bool() const { return bytes != nullptr; }
};
} // namespace facade
