#include "json.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <cmath>
#include <functional>
#include <iostream>

#include "util/error_handling.h"
#include "util/file_descriptor.h"

namespace serial::json {

inline uint8_t from_hexchar(char c)
{
	uint8_t rc = 0;

	if (c >= '0' && c <= '9')
		rc = c - '0';
	else if (c >= 'A' && c <= 'F')
		rc = 10 + c - 'A';
	else if (c >= 'a' && c <= 'f')
		rc = 10 + c - 'a';

	return rc;
}

inline void to_utf8(char32_t codoepoint, std::string & destination)
{
	if (codoepoint < 0x80)
	{
		destination.push_back(codoepoint);
	} else if (codoepoint < 0x800)
	{
		destination.reserve(2);
		destination.push_back( 0xc0 | ( (codoepoint >>  6) & 0x1f ) );
		destination.push_back( 0x80 | ( (codoepoint >>  0) & 0x3f ) );
	} else if (codoepoint < 0x10000)
	{
		destination.reserve(3);
		destination.push_back( 0xe0 | ( (codoepoint >> 12) & 0x0f ) );
		destination.push_back( 0x80 | ( (codoepoint >>  6) & 0x3f ) );
		destination.push_back( 0x80 | ( (codoepoint >>  0) & 0x3f ) );
	} else if (codoepoint < 0x110000)
	{
		destination.reserve(4);
		destination.push_back( 0xf0 | ( (codoepoint >> 18) & 0x07 ) );
		destination.push_back( 0x80 | ( (codoepoint >> 12) & 0x3f ) );
		destination.push_back( 0x80 | ( (codoepoint >>  6) & 0x3f ) );
		destination.push_back( 0x80 | ( (codoepoint >>  0) & 0x3f ) );
	} else
		throw std::runtime_error("unicode code point is too large");
}

%%{
	machine json;

	action print_transition {
//		printf ("(%u, '%c')\n", cs, *p);
	}

	prepush {
//		printf("PUSH\n");
		stack.push_back(0);
	}

	postpop {
//		printf("POP (%u), c='%c'\n", stack.back(), *p);
		stack.pop_back();
//		printf("POP size = %zu, top = %u, cs = %u, c='%c'\n",
//		       stack.size(), stack.back(), cs, *p);
	}

	action string_start {
		token_buffer.clear();
	}
	action string_append {
		token_buffer.push_back(*p);
	}
	action string_append_escape {
		switch (*p)
		{
		 case '"': token_buffer.push_back('"'); break;
		 case '\\': token_buffer.push_back('\\'); break;
		 case 'b': token_buffer.push_back('\b'); break;
		 case 'f': token_buffer.push_back('\f'); break;
		 case 'n': token_buffer.push_back('\n'); break;
		 case 'r': token_buffer.push_back('\r'); break;
		 case 't': token_buffer.push_back('\t'); break;
		 default:
			throw std::runtime_error("bad escape");
			break;
		}
	}
	action string_end {
//		printf(">>> %s\n", token_buffer.c_str());
	}
	action unicode_escape_char {
		integer_buffer <<= 4;
		integer_buffer |= from_hexchar(*p);
	}
	action reset_integer_buffer {
		integer_buffer = 0;
		exponent = 0;
		negative_exponent = false;
		fraction_shift = 0;
	}
	action save_unicode {
		to_utf8(integer_buffer, token_buffer);
	}
	action state_return {
//		printf("returning (%zu)\n", stack.size());
		fret;
	}
	action recurse {
//		printf("recursing on c = '%c'\n", *p);
		fcall json_value;
	}
	action hold_recurse {
		fhold;
		fcall json_value;
	}
	action push_index {
		object_path.push_back(0);
	}
	action push_key {
		object_member_count = 0;
		token_buffer.clear();
		object_path.push_back(token_buffer);
	}
	action pop_index {
		object_path.pop_back();
	}
	action pop_key {
		object_path.pop_back();
		if (object_member_count == 0)
			data.add_datum(object_path, empty_object{});
	}
	action increment_path_index {
		++std::get<uint64_t>(object_path.back());
	}
	action update_key {
		std::get<std::string>(object_path.back()) = token_buffer;
	}
	action process_value {
		data.add_datum(object_path, std::move(token_buffer));
	}
	action hold_char { fhold; }

	action accumulate_fraction {
		integer_buffer *= 10;
		integer_buffer += *p - '0';
		--fraction_shift;
	}
	action clear_sign { negative = false; }
	action negate_value { negative = true; }
	action accumulate_int {
		integer_buffer *= 10;
		integer_buffer += *p - '0';
	}

	action accumulate_exponent {
		exponent *= 10;
		exponent += *p - '0';
	}

	action negate_exponent {
		negative_exponent = true;
	}

	action publish_number {
		if (negative_exponent)
			exponent = -exponent;

		if ((exponent + fraction_shift) == 0)
		{
			if (negative)
			{
				int64_t x = -integer_buffer;
				data.add_datum(object_path, x);
			} else
			{
				data.add_datum(object_path, integer_buffer);
			}
		} else
		{
			exponent += fraction_shift;

			double value = integer_buffer;
			value *= std::pow<double>(10, exponent);

			if (negative)
				value = -value;

			data.add_datum(object_path, value);
		}
	}
	action publish_true {
		data.add_datum(object_path, true);
	}
	action publish_false {
		data.add_datum(object_path, false);
	}
	action publish_null {
		data.add_datum(object_path, nullptr);
	}

	ws = (0x20 | 0x0a @ { ++line_number; } | 0x0d | 0x09);

	unicode_hexdigit = [0-9a-fA-F] @ unicode_escape_char;

	escaped_character = '\\' ["\/bfnrt] $ string_append_escape;

	action save_be_surrogate {
		uint32_t tmp = (integer_buffer >> 6) & 0x000ffc00;
		tmp |= (integer_buffer & 0x0000003ff);
		tmp += 0x10000;
		to_utf8(tmp, token_buffer);
	}
	action save_le_surrogate {
		uint32_t tmp = (integer_buffer >> 16) & 0x000003ff;
		tmp |= ((integer_buffer << 10) & 0x0000ffc00);
		tmp += 0x10000;
		to_utf8(tmp, token_buffer);
	}

	unicode_escape =
		'\\' 'u' > reset_integer_buffer
		( ( ( ( [0-9a-cefA-CEF][0-9a-fA-F]{3} )
		    | ( [dD][0-7][0-9a-fA-F]{2} ) ) )
		  $ unicode_escape_char
		  % save_unicode
		| ( ( [dD][89aAbB][0-9a-fA-F]{2} ) $ unicode_escape_char
		    '\\' 'u' ( [dD][c-fC-F][0-9a-fA-F]{2} ) $ unicode_escape_char )
		  % save_be_surrogate
		| ( ( [dD][c-fC-F][0-9a-fA-F]{2} ) $ unicode_escape_char
		    '\\' 'u' ( [dD][89aAbB][0-9a-fA-F]{2} ) $unicode_escape_char )
		  % save_le_surrogate
		)
		;

	string_character =
		( ( (0x00 .. 0x7f) - ( '\\' | '"' ) )
		| ( (0xc0 .. 0xdf) (0x80 .. 0xbf) )
		| ( (0xe0 .. 0xef) (0x80 .. 0xbf){2} )
		| ( (0xf0 .. 0xf7) (0x80 .. 0xbf){3} )
		) $ string_append;

	string_characters =
		( string_character
		| escaped_character
		| unicode_escape )*;

	string = '"' > string_start string_characters '"' $ string_end;

	action label_start {
		std::get<std::string>(object_path.back()).clear();
		++object_member_count;
	}
	action label_append {
		std::get<std::string>(object_path.back()).push_back(*p);
	}
	action label_append_escape {
		std::string & s = std::get<std::string>(object_path.back());
		switch (*p)
		{
		 case '"': s.push_back('"'); break;
		 case '\\': s.push_back('\\'); break;
		 case 'b': s.push_back('\b'); break;
		 case 'f': s.push_back('\f'); break;
		 case 'n': s.push_back('\n'); break;
		 case 'r': s.push_back('\r'); break;
		 case 't': s.push_back('\t'); break;
		 default:
			throw std::runtime_error("bad escape");
			break;
		}
	}
	action label_save_unicode {
		to_utf8(integer_buffer, std::get<std::string>(object_path.back()));
	}

	action label_save_be_surrogate {
		uint32_t tmp = (integer_buffer >> 6) & 0x000ffc00;
		tmp |= (integer_buffer & 0x0000003ff);
		tmp += 0x10000;
		to_utf8(tmp, std::get<std::string>(object_path.back()));
	}
	action label_save_le_surrogate {
		uint32_t tmp = (integer_buffer >> 16) & 0x000003ff;
		tmp |= ((integer_buffer << 10) & 0x0000ffc00);
		tmp += 0x10000;
		to_utf8(tmp, std::get<std::string>(object_path.back()));
	}

	label_unicode_hexdigit = [0-9a-fA-F] @ unicode_escape_char;

	label_escaped_character = '\\' ["\/bfnrt] $ label_append_escape;

	label_unicode_escape =
		'\\' 'u' > reset_integer_buffer
		( ( ( ( [0-9a-cefA-CEF][0-9a-fA-F]{3} )
		    | ( [dD][0-7][0-9a-fA-F]{2} ) ) )
		  $ unicode_escape_char
		  % label_save_unicode
		| ( ( [dD][89aAbB][0-9a-fA-F]{2} ) $ unicode_escape_char
		    '\\' 'u' ( [dD][c-fC-F][0-9a-fA-F]{2} )
		    $ unicode_escape_char )
		  % label_save_be_surrogate
		| ( ( [dD][c-fC-F][0-9a-fA-F]{2} ) $ unicode_escape_char
		    '\\' 'u' ( [dD][89aAbB][0-9a-fA-F]{2} )
		    $ unicode_escape_char )
		  % label_save_le_surrogate
		)
		;

	label_string_character =
		( ( (0x00 .. 0x7f) - ( '\\' | '"' ) )
		| ( (0xc0 .. 0xdf) (0x80 .. 0xbf) )
		| ( (0xe0 .. 0xef) (0x80 .. 0xbf){2} )
		| ( (0xf0 .. 0xf7) (0x80 .. 0xbf){3} )
		) $ label_append;

	label_string_characters =
		( label_string_character
		| label_escaped_character
		| label_unicode_escape )*;

	label = '"' > label_start label_string_characters '"';

	one_to_nine = [1-9];

	sign = '-';

	digits = digit+;

	integer = one_to_nine . digits* | '0';

	fraction = digit*;

	exponent = [eE] ('+' | '-' @ negate_exponent)? digits @ accumulate_exponent;

	number =
		sign? > clear_sign @ negate_value
		integer > reset_integer_buffer @ accumulate_int
		( '.' fraction @ accumulate_fraction )?
		exponent ?
		(ws | ',' | ']' | '}') @ hold_char;

	value_start_char = [[{\-0-9tfn\"];

	action publish_empty_array {
			data.add_datum(object_path, empty_array{ });
	}

	action publish_empty_object {
			data.add_datum(object_path, empty_object{ });
	}

	array =
		( ( '[' ws* ']' ) @ publish_empty_array
		| ( '[' ws* value_start_char > push_index  @ hold_recurse
		( ws* ',' ws* value_start_char > increment_path_index @hold_recurse )*
		ws* ']' > pop_index ) );

	object_element =
		ws* label ws* ':' @recurse;

	object =
		'{' @ push_key
		( ws*
		| object_element
		| object_element ( ',' object_element )* )
		ws* '}' @ pop_key;

	boolean = "true" @ publish_true | "false" @ publish_false;

	null_value = "null" @ publish_null;

	json =
		ws*
		( string @ process_value
		| number @ publish_number
		| null_value
		| boolean
		| array
		| object
		) ws*;

	json_value := json @ state_return;

	main := (
		json $from(print_transition)
	);
}%%

%%write data;

parser::parser()
  : cs(0)
  , top(0)
  , integer_buffer(0)
  , eof(nullptr)
  , exponent(0)
  , fraction_shift(0)
  , object_member_count(0)
  , line_number(1)
  , token_buffer()
  , stack()
  , object_path()
  , negative_exponent(false)
  , negative(false)
{
	%%write init;
}

void parser::read_file(const stdfs::path & filename, data_visitor & data)
{
	util::file_descriptor fd = (
		filename == "-" ? dup(0) : open(filename.c_str(), O_RDONLY) );

	if (fd < 0)
		util::throw_errno("Could not open file '%s'", filename.c_str());

	struct stat st;
	fstat(fd, &st);

	std::unique_ptr<void, std::function<void(void*)>> ptr(
		mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0),
		[&](void * x) { if (x != MAP_FAILED) munmap(x, st.st_size); });

	if (ptr.get() == MAP_FAILED)
	{
		static constexpr size_t bufsz = 16384;
		char buffer[bufsz];
		ssize_t n = 0;

		while ((n = read(fd, buffer, bufsz)) > 0)
			parse_data(buffer, n, data);
	} else
	{
		parse_data(static_cast<char *>(ptr.get()), st.st_size, data);
	}
}

void parser::parse_data(const char * buffer, size_t n, data_visitor & data)
{
	const char * p = buffer;
	const char * pe = p + n;

	%%write exec;

	if (cs <= json_error)
	{
		std::cerr << "cs = " << cs << ", pos = '" << p << "'\n";
		throw std::runtime_error("Parse failed\n");
	} else if ( cs >= json_first_final )
	{
		std::cerr << "Parse success, " << (line_number - 1) << " lines read\n";
	}
}

} // namespacee serial::json
