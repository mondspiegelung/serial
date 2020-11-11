#include "error_handling.h"

#include <cstdarg>
#include <string>
#include <system_error>

namespace util {

[[noreturn]]
void throw_errno(const char * fmt, ...)
{
	std::string msg;
	va_list ap;

	va_start(ap, fmt);
	int size = vsnprintf(nullptr, 0, fmt, ap);
	va_end(ap);

	if (size < 0 )
		throw std::runtime_error("Bad format string");

	msg.assign(size, '_');

	va_start(ap, fmt);
	size = vsnprintf(msg.data(), msg.size() + 1, fmt, ap);
	va_end(ap);

	if (size < 0 )
		throw std::runtime_error("Bad format string");

	throw std::system_error(errno, std::system_category(), msg);
}

} // namespace util
