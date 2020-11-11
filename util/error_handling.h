#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

namespace util {

[[noreturn]]
void throw_errno(const char * fmt, ...);

}

#endif // ERROR_HANDLING_H
