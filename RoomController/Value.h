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

template<class T> bool is(const value_t& v) { return std::get_if<T>(&v); }
template<class T> T as(const value_t& v) { return std::get<T>(v); }
inline bool is_error(const value_t& v) { return std::get_if<error_t>(&v); }
inline bool is_sensor(const value_t& v) { return std::get_if<i_sensor*>(&v); }
template<class T = value_type> T get_arg(const params_t& params, unsigned idx) {  
	if(idx >= params.size())	throw error_t::invalid_args;
	return T(as<value_type>(*params[idx]));
}

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
	virtual void start() = 0;
	virtual void open() = 0;
	virtual void stop() = 0;
	virtual void close() = 0;
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
	if(const auto ps = std::get_if<i_sensor*>(&v))	return (*ps)->value();
	return v;
}

inline value_t operator +(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) + std::get<value_type>(*rhs); }
inline value_t operator -(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) - std::get<value_type>(*rhs); }
inline value_t operator *(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) * std::get<value_type>(*rhs); }
inline value_t operator /(const value_t& lhs, const value_t& rhs) { return std::get<value_type>(*lhs) / std::get<value_type>(*rhs); }

}