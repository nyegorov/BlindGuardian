#pragma once

#include "Value.h"

namespace BlindGuardian
{

using std::string;
using std::vector;
#undef min
#undef max

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
	virtual void activate(value_t value) const = 0;
};

struct IActuator
{
	virtual string name() const = 0;
	virtual vector<const IAction*> actions() const = 0;
};

class action : public IAction
{
	string		_name;
	std::function<void(value_t)> _action;
public:
	action(const char *name, std::function<void(value_t)> func) : _name(name), _action(func) {}
	string name() const override { return _name; }
	void activate(value_t value) const override { _action(value); }
};

class actuator : public IActuator
{
	string _name;
public:
	actuator(const char *name) : _name(name) { }
	string name() const override { return _name; };
};

class sensor : public ISensor
{
protected:
	value_t _value;
	value_t _min;
	value_t _max;
	string _name;
public:
	sensor(const char *name, value_tag type) : _name(name), _value{ type, 0 } { reset(); }
	value_t value() const override { return _value; }
	value_t min() const override { return _min;	}
	value_t max() const override { return _max; }
	void reset() override { _min = _max = _value; }
	string name() const override { return _name; }
};

class TemperatureSensor : public sensor
{
public:
	TemperatureSensor(const char *name) : sensor(name, value_tag::temperature) { }
	void update() override { };
};

class LightSensor : public sensor
{
public:
	LightSensor(const char *name) : sensor(name, value_tag::light) { }
	void update() override { };
};

class time_sensor : public sensor
{
public:
	time_sensor(const char *name) : sensor(name, value_tag::time) { }
	void update() override {
		auto now = std::chrono::system_clock::now();
		auto now_t = std::chrono::system_clock::to_time_t(now);
		tm tm;
		localtime_s(&tm, &now_t);
		_value.value = tm.tm_hour * 60 + tm.tm_min;
	};
};

}