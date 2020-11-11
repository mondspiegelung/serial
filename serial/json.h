#ifndef JSON_H
#define JSON_H 1

#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <charconv>

#include "data_visitor.h"

namespace serial::json {

namespace stdfs = std::filesystem;

class parser
{
 public:
	parser();

	void read_file(const stdfs::path & filename, data_visitor & data);

 private:
	void parse_data(const char * buffer, size_t n, data_visitor & data);

	unsigned cs;
	unsigned top;
	uint64_t integer_buffer;
	char * eof;
	signed exponent;
	signed fraction_shift;
	unsigned object_member_count;
	unsigned line_number;
	std::string token_buffer;
	std::vector<unsigned> stack;
	std::vector<std::variant<std::string, uint64_t>> object_path;
	bool negative_exponent;
	bool negative;
};

} // namespace serial::json

#endif // JSON_H
