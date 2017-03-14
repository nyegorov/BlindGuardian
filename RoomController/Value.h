#pragma once

namespace BlindGuardian
{

#undef min
#undef max
using std::string;
using std::wstring;
using std::vector;

struct i_sensor;
enum class error_t { cast, invalid_args, not_implemented, syntax, name_not_found, type_mismatch, runtime, empty };
using value_type = int32_t;

using value_t = std::variant<value_type, i_sensor*, error_t>;
using params_t = vector<value_t>;

inline bool is_error(value_t& v) { return std::get_if<error_t>(&v); }

struct i_sensor
{
	virtual string name() const = 0;
	virtual value_t value() const = 0;
	virtual value_t min() const = 0;
	virtual value_t max() const = 0;
	virtual void reset() = 0;
	virtual void update() = 0;
};

struct i_action
{
	virtual string name() const = 0;
	virtual void activate(const params_t& params) const = 0;
};

struct i_actuator
{
	virtual string name() const = 0;
	virtual vector<const i_action*> actions() const = 0;
};

inline value_t operator *(const value_t& v)
{
	if(auto ps = std::get_if<i_sensor*>(&v))	return (*ps)->value();
	return v;
}

inline value_t operator +(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) + std::get<value_type>(*rhs); }
inline value_t operator -(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) - std::get<value_type>(*rhs); }
inline value_t operator *(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) * std::get<value_type>(*rhs); }
inline value_t operator /(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) / std::get<value_type>(*rhs); }

}