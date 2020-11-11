#include "file_descriptor.h"

#include <unistd.h>

#include <utility>

namespace util {

file_descriptor::file_descriptor()
  : fd(-1)
	{ }

file_descriptor::file_descriptor(int fd)
  : fd(fd)
	{ }

file_descriptor::file_descriptor(file_descriptor && other)
  : fd(-1)
{
	std::swap(fd, other.fd);
}

file_descriptor & file_descriptor::operator = (file_descriptor && other)
{
	std::swap(fd, other.fd);
	return *this;
}

file_descriptor::~file_descriptor() { this->close(); }

void file_descriptor::close()
{
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
}

file_descriptor::operator int() const { return fd; }

} // namespace util
