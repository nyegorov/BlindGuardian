#pragma once

namespace BlindGuardian
{

#undef min
#undef max
using std::string;
using std::wstring;
using std::vector;

struct value_t;
using params_t = vector<value_t>;

struct ISensor
{
	virtual string name() const = 0;
	virtual value_t value() const = 0;
	virtual value_t min() const = 0;
	virtual value_t max() const = 0;
	virtual void reset() = 0;
	virtual void update() = 0;
};

struct IAction
{
	virtual string name() const = 0;
	virtual void activate(const params_t& params) const = 0;
};

struct IActuator
{
	virtual string name() const = 0;
	virtual vector<const IAction*> actions() const = 0;
};

enum class value_tag { value, temperature, light, time, callback, error };
enum class error_t { cast, invalid_args, not_implemented, syntax, name_not_found, type_mismatch, empty };
enum class compare_t { less = -1, equal = 0, greater = 1 };
using value_type = int64_t;

struct value_t
{
	value_tag  type = value_tag::value;
	value_type value = 0;

	value_t() : value_t(value_tag::value, 0) {}
	value_t(value_type value) : value_t(value_tag::value, value) {}
	value_t(ISensor& value) : value_t(value_tag::callback, reinterpret_cast<value_type>(&value)) {}
	value_t(value_tag type, value_type value) : type(type), value(value) {}

	operator value_type() const { return value; }
};

inline value_t operator *(value_t v)
{
	return v.type == value_tag::callback ? reinterpret_cast<ISensor*>(v.value)->value() : v;
}

inline value_tag check_args(value_t& lhs, value_t& rhs)
{
	lhs = *lhs;
	rhs = *rhs;
	if(lhs.type != rhs.type && lhs.type != value_tag::value && rhs.type != value_tag::value)	throw error_t::type_mismatch;
	return lhs.type == value_tag::value ? rhs.type : lhs.type;
}

inline value_t operator +(value_t lhs, value_t rhs)
{
	auto type = check_args(lhs, rhs);
	return value_t{ type, lhs.value + rhs.value };
}

inline value_t operator -(value_t lhs, value_t rhs)
{
	auto type = check_args(lhs, rhs);
	return value_t{ type, lhs.value - rhs.value };
}

inline value_t operator *(value_t lhs, value_t rhs)
{
	auto type = check_args(lhs, rhs);
	return value_t{ type, lhs.value * rhs.value };
}

inline value_t operator /(value_t lhs, value_t rhs)
{
	auto type = check_args(lhs, rhs);
	return value_t{ type, lhs.value / rhs.value };
}

inline compare_t compare(value_t lhs, value_t rhs)
{
	check_args(lhs, rhs);
	return lhs.value == rhs.value ? compare_t::equal :
		lhs.value < rhs.value ? compare_t::less : compare_t::greater;
}

inline bool operator ==(value_t lhs, value_t rhs) {	return compare(lhs, rhs) == compare_t::equal; }

}