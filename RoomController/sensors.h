#pragma once

#include "value.h"

namespace roomctrl
{

class action : public i_action
{
	wstring		_name;
	std::function<void(const params_t&)> _action;
public:
	action(const wchar_t *name, std::function<void(const params_t&)> func) : _name(name), _action(func) {}
	wstring name() const override { return _name; }
	void activate(const params_t& params) const override { _action(params); }
	void operator() (const params_t& params) const { activate(params); }
	void operator() () const { activate(params_t{}); }
};

class actuator : public i_actuator
{
	wstring _name;
public:
	actuator(const wchar_t *name) : _name(name) { }
	wstring name() const override { return _name; };
};

class sensor : public i_sensor
{
protected:
	value_t _value;
	value_t _min;
	value_t _max;
	wstring _name;
public:
	sensor(const wchar_t *name, value_t value = value_type{ 0 }) : _name(name), _value{ value } { reset(); }
	value_t value() const override { return _value; }
	value_t min() const override { return _min;	}
	value_t max() const override { return _max; }
	void reset() override { _min = _max = _value; }
	wstring name() const override { return _name; }
};

class time_sensor : public sensor
{
public:
	time_sensor(const wchar_t *name) : sensor(name) { update(); }
	void update() override {
		auto now = std::chrono::system_clock::now();
		auto now_t = std::chrono::system_clock::to_time_t(now);
		tm tm;
		localtime_s(&tm, &now_t);
		_value = tm.tm_hour * 60 + tm.tm_min;
	};
};

class remote_sensor : public sensor
{
	std::future<value_t> value_async();
	uint8_t	_cmdbyte;
public:
	remote_sensor(const wchar_t *name, uint8_t cmd_byte) : sensor(name), _cmdbyte(cmd_byte) { }
	void update() override;

};

}