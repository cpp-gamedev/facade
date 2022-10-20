#pragma once
#include <facade/util/byte_buffer.hpp>
#include <string>

namespace facade {
struct DataProvider {
	virtual ByteBuffer load(std::string_view uri) const = 0;
};

class FileDataProvider : public DataProvider {
  public:
	static FileDataProvider mount_parent_dir(std::string_view filename);

	FileDataProvider(std::string_view directory);

	ByteBuffer load(std::string_view uri) const override;

	explicit operator bool() const { return !m_directory.empty(); }

  private:
	std::string m_directory{};
};
} // namespace facade
