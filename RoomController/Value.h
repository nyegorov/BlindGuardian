#pragma once

namespace roomctrl
{

#undef min
#undef max
using std::wstring;
using std::wstring_view;
using std::vector;

struct i_sensor;
enum class error_t { invalid_args, not_implemented, syntax_error, name_not_found, type_mismatch, runtime };
using value_type = int32_t;

using value_t = std::variant<value_type, i_sensor*, error_t>;
using params_t = vector<value_t>;

inline bool is_error(const value_t& v) { return std::get_if<error_t>(&v); }
inline bool is_sensor(const value_t& v) { return std::get_if<i_sensor*>(&v); }

struct i_sensor
{
	virtual wstring name() const = 0;
	virtual value_t value() const = 0;
	virtual value_t min() const = 0;
	virtual value_t max() const = 0;
	virtual void reset() = 0;
	virtual void update() = 0;
};

struct i_motor {
	virtual void open() = 0;
	virtual void close() = 0;
	virtual void setpos(value_t pos) = 0;
};

struct i_action
{
	virtual wstring name() const = 0;
	virtual void activate(const params_t& params) const = 0;
};

struct i_actuator
{
	virtual wstring name() const = 0;
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