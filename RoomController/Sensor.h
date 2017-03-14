#pragma once

#include "value.h"

namespace BlindGuardian
{

class action : public i_action
{
	string		_name;
	std::function<void(const params_t&)> _action;
public:
	action(const char *name, std::function<void(params_t)> func) : _name(name), _action(func) {}
	string name() const override { return _name; }
	void activate(const params_t& params) const override { _action(params); }
	void operator() (const params_t& params) const { activate(params); }
	void operator() () const { activate(params_t{}); }
};

class actuator : public i_actuator
{
	string _name;
public:
	actuator(const char *name) : _name(name) { }
	string name() const override { return _name; };
};

class sensor : public i_sensor
{
protected:
	value_t _value;
	value_t _min;
	value_t _max;
	string _name;
public:
	sensor(const char *name, value_t value = value_type{ 0 }) : _name(name), _value{ value } { reset(); }
	value_t value() const override { return _value; }
	value_t min() const override { return _min;	}
	value_t max() const override { return _max; }
	void reset() override { _min = _max = _value; }
	string name() const override { return _name; }
};

class time_sensor : public sensor
{
public:
	time_sensor(const char *name) : sensor(name) { }
	void update() override {
		auto now = std::chrono::system_clock::now();
		auto now_t = std::chrono::system_clock::to_time_t(now);
		tm tm;
		localtime_s(&tm, &now_t);
		_value = tm.tm_hour * 60 + tm.tm_min;
	};
};

}