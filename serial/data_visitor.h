#ifndef DATA_VISITOR_H
#define DATA_VISITOR_H 1

#include <iosfwd>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>
#include <cstdint>

namespace serial {

using path = std::vector<std::variant<std::string, uint64_t>>;

struct empty_array { };

struct empty_object { };

class data_visitor
{
 public:
	data_visitor() { }

	virtual ~data_visitor() { }

	virtual void add_datum(const path & p, const empty_array &) = 0;

	virtual void add_datum(const path & p, const empty_object &) = 0;

	virtual void add_datum(const path & p, std::nullptr_t) = 0;

	virtual void add_datum(const path & p, bool) = 0;

	virtual void add_datum(const path & p, int64_t) = 0;

	virtual void add_datum(const path & p, uint64_t) = 0;

	virtual void add_datum(const path & p, double) = 0;

	virtual void add_datum(const path & p, std::string &&) = 0;
};

class printing_visitor : public data_visitor
{
 public:
	printing_visitor() : data_visitor() { }

	~printing_visitor() { }

	void add_datum(const path & p, const empty_array &) override;

	void add_datum(const path & p, const empty_object &) override;

	void add_datum(const path & p, std::nullptr_t) override;

	void add_datum(const path & p, bool) override;

	void add_datum(const path & p, int64_t) override;

	void add_datum(const path & p, uint64_t) override;

	void add_datum(const path & p, double) override;

	void add_datum(const path & p, std::string &&) override;

 protected:
	void print_path(std::ostream & out, const path & object_path);
};

struct value
{
	using string_type = std::string;
	using array_type = std::vector<value>;
	using array_ptr_type = std::shared_ptr<array_type>;
	using object_type = std::unordered_map<std::string, value>;
	using object_ptr_type = std::shared_ptr<object_type>;

	value();

	value(const value & other);

	value(value && other);

	value & operator = (const value & other);

	value & operator = (value && other);

	virtual ~value();

	std::variant<int64_t,
	             uint64_t,
	             double,
	             bool,
	             std::nullptr_t,
	             string_type,
	             empty_array,
	             std::shared_ptr<array_type>,
	             empty_object,
	             std::shared_ptr<object_type>> datum;

	void print(std::ostream & out,
	           unsigned indent = 0,
	           bool indent_first = true) const;
};

class value_store : public value, public data_visitor
{
 public:
	value * walk_path(const path & object_path)
	{
		value * current = this;

		for (const auto & p : object_path)
		{
			if (std::holds_alternative<std::string>(p))
			{
				auto key = std::get<std::string>(p);

				if ( ! std::holds_alternative<object_ptr_type>(current->datum)
				   || std::get<object_ptr_type>(current->datum) == nullptr )
					current->datum.emplace<object_ptr_type>(new object_type);

				object_type & o = *std::get<object_ptr_type>(current->datum);

				current = &o[key];

			} else if (std::holds_alternative<uint64_t>(p))
			{
				auto index = std::get<uint64_t>(p);

				if ( ! std::holds_alternative<array_ptr_type>(current->datum)
				   || std::get<array_ptr_type>(current->datum) == nullptr )
					current->datum.emplace<array_ptr_type>(new array_type);

				array_type & a = *std::get<array_ptr_type>(current->datum);

				a.resize(index + 1);

				current = &a[index];

			} else
			{
				throw std::runtime_error("path holds unknown type");
			}
		}

		return current;
	}

	void add_datum(const path & object_path, const empty_array & a) override
	{
		value * current = walk_path(object_path);
		current->datum = a;
	}

	void add_datum(const path & object_path, const empty_object & o) override
	{
		value * current = walk_path(object_path);
		current->datum = o;
	}

	void add_datum(const path & object_path, std::nullptr_t) override
	{
		value * current = walk_path(object_path);
		current->datum = nullptr;
	}

	void add_datum(const path & object_path, bool datum) override
	{
		value * current = walk_path(object_path);
		current->datum = std::move(datum);
	}

	void add_datum(const path & object_path, int64_t datum) override
	{
		value * current = walk_path(object_path);
		current->datum = std::move(datum);
	}

	void add_datum(const path & object_path, uint64_t datum) override
	{
		value * current = walk_path(object_path);
		current->datum = std::move(datum);
	}

	void add_datum(const path & object_path, double datum) override
	{
		value * current = walk_path(object_path);
		current->datum = std::move(datum);
	}

	void add_datum(const path & object_path, std::string && datum) override
	{
		value * current = walk_path(object_path);
		current->datum = std::move(datum);
	}
};

} // namespace serial

#endif // DATA_VISITOR_H
