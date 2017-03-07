#pragma once

namespace BlindGuardian
{

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
	value_t(value_tag type, value_type value) : type(type), value(value) {}

	operator value_type() const { return value; }
};

inline value_t operator +(value_t lhs, value_t rhs)
{
	if(lhs.type == rhs.type || lhs.type == value_tag::value)		return value_t{ rhs.type, lhs.value + rhs.value };
	if(rhs.type == value_tag::value)								return value_t{ lhs.type, lhs.value + rhs.value };
	throw error_t::type_mismatch;
}

inline value_t operator -(value_t lhs, value_t rhs)
{
	if(lhs.type == rhs.type || lhs.type == value_tag::value)		return value_t{ rhs.type, lhs.value - rhs.value };
	if(rhs.type == value_tag::value)								return value_t{ lhs.type, lhs.value - rhs.value };
	throw error_t::type_mismatch;
}

inline value_t operator *(value_t lhs, value_t rhs)
{
	if(lhs.type == rhs.type || lhs.type == value_tag::value)		return value_t{ rhs.type, lhs.value * rhs.value };
	if(rhs.type == value_tag::value)								return value_t{ lhs.type, lhs.value * rhs.value };
	throw error_t::type_mismatch;
}

inline value_t operator /(value_t lhs, value_t rhs)
{
	if(lhs.type == rhs.type || lhs.type == value_tag::value)		return value_t{ rhs.type, lhs.value / rhs.value };
	if(rhs.type == value_tag::value)								return value_t{ lhs.type, lhs.value / rhs.value };
	throw error_t::type_mismatch;
}

inline compare_t compare(value_t lhs, value_t rhs)
{
	if(lhs.type != rhs.type && lhs.type != value_tag::value && rhs.type != value_tag::value)	throw error_t::type_mismatch;
	return lhs.value == rhs.value ? compare_t::equal :
		lhs.value < rhs.value ? compare_t::less : compare_t::greater;
}

inline bool operator ==(value_t lhs, value_t rhs) {	return compare(lhs, rhs) == compare_t::equal; }

}