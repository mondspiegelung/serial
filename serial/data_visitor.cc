#include "data_visitor.h"

#include <iostream>
#include <iomanip>

namespace serial {

void printing_visitor::print_path(std::ostream & out, const path & object_path)
{
	for (const auto & p : object_path)
		std::visit([&](auto && arg) { out << '.' << arg; }, p);
}

void printing_visitor::add_datum(const path & object_path, const empty_array &)
{
	print_path(std::cout, object_path);
	std::cout << " -> [ ]\n";
}

void printing_visitor::add_datum(const path & object_path, const empty_object &)
{
	print_path(std::cout, object_path);
	std::cout << " -> { }\n";
}

void printing_visitor::add_datum(const path & object_path, std::nullptr_t)
{
	print_path(std::cout, object_path);
	std::cout << " -> " << "null\n";
}

void printing_visitor::add_datum(const path & object_path, bool datum)
{
	print_path(std::cout, object_path);
	std::cout << " -> " << std::boolalpha << datum << '\n';
}

void printing_visitor::add_datum(const path & object_path, int64_t datum)
{
	print_path(std::cout, object_path);
	std::cout << " -> " << datum << '\n';
}

void printing_visitor::add_datum(const path & object_path, uint64_t datum)
{
	print_path(std::cout, object_path);
	std::cout << " -> " << datum << '\n';
}

void printing_visitor::add_datum(const path & object_path, double datum)
{
	print_path(std::cout, object_path);
	std::cout << " -> " << datum << '\n';
}

void printing_visitor::add_datum(const path & object_path, std::string && datum)
{
	print_path(std::cout, object_path);
	std::cout << " -> " << datum << '\n';
}

//////////////////////////////////////////////////////////////////////
value::value() : datum() { }

value::value(const value & other) : datum(other.datum) { }

value::value(value && other) : datum(std::move(other.datum)) { }

value & value::operator = (const value & other) {
	datum = other.datum;
	return *this;
}

value & value::operator = (value && other) {
	std::swap(datum, other.datum);
	return *this;
}

value::~value() { }

void value::print(std::ostream & out,
                  unsigned indent,
                  bool indent_first) const
{
	if (indent_first)
		out << std::string(indent, '\t');

	if (std::holds_alternative<string_type>(datum))
	{
		out << '\"';
		for (auto c : std::get<std::string>(datum))
			switch (c) {
			 case '\"': out << "\\\""; break;
			 case '\\': out << "\\\\"; break;
			 case '\b': out << "\\b"; break;
			 case '\f': out << "\\f"; break;
			 case '\n': out << "\\n"; break;
			 case '\r': out << "\\r"; break;
			 case '\t': out << "\\t"; break;
			 default: out.put(c); break;
			}
		out << '\"';
	} else if (std::holds_alternative<int64_t>(datum))
	{
		out << std::get<int64_t>(datum);
	} else if (std::holds_alternative<uint64_t>(datum))
	{
		out << std::get<uint64_t>(datum);
	} else if (std::holds_alternative<double>(datum))
	{
		out << std::setprecision(std::numeric_limits<double>::digits10 + 1)
		    << std::scientific
		    << std::get<double>(datum);
	} else if (std::holds_alternative<bool>(datum))
	{
		out << std::boolalpha
		    << std::get<bool>(datum);
	} else if (std::holds_alternative<std::nullptr_t>(datum))
	{
		out << "null";
	} else if (std::holds_alternative<empty_array>(datum))
	{
		out << "[ ]";
	} else if (std::holds_alternative<array_ptr_type>(datum))
	{
		const array_type & a = *std::get<array_ptr_type>(datum);
		out << "[\n";

		auto i = a.begin();

		if (i != a.end())
		{
			i->print(out, indent + 1);
			++i;
		}

		for (; i != a.end(); ++i)
		{
			out << ",\n";
			i->print(out, indent + 1);
		}

		out << '\n';

		out << std::string(indent, '\t') << "]";
	} else if (std::holds_alternative<empty_object>(datum))
	{
		out << "{ }";
	} else if (std::holds_alternative<object_ptr_type>(datum))
	{
		const object_type & o = *std::get<object_ptr_type>(datum);
		out << "{\n";

		auto i = o.begin();

		if (i != o.end())
		{
			out << std::string(indent + 1, '\t') << '"' << i->first
			    << "\": ";
			i->second.print(out, indent + 1, false);
			++i;
		}

		for (; i != o.end(); ++i)
		{
			out << ",\n" << std::string(indent + 1, '\t')
			    << '"' << i->first << "\": ";

			i->second.print(out, indent + 1, false);
		}

		out << '\n';

		out << std::string(indent, '\t') << "}";
	}
}

} // namespace serial
