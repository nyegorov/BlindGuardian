#pragma once

#include "Value.h"

namespace BlindGuardian
{

using std::string;
using std::vector;

struct ISensor
{
	virtual string name() const = 0;
	virtual value_t value() const = 0;
	virtual void update() = 0;
};

struct IAction
{
	virtual string name() = 0;
	virtual void activate(value_t value) = 0;
};

struct IActuator
{
	virtual string name() = 0;
	virtual vector<IAction*> actions() = 0;
};

class action : public IAction
{
	string		_name;
	std::function<void(value_t)> _action;
public:
	action(const char *name, std::function<void(value_t)> func) : _name(name), _action(func) {}
	string name() { return _name; }
	void activate(value_t value) { _action(value); }
};

class actuator : public IActuator
{
	string _name;
public:
	actuator(const char *name) : _name(name) { }
	string name() { return _name; };
};

class sensor : public ISensor
{
protected:
	value_t _value;
	string _name;
public:
	sensor(const char *name, value_tag type) : _name(name), _value{ type, 0 } {}
	value_t value() const { return _value; }
	string name() const { return _name; }
};

class TemperatureSensor : public sensor
{
public:
	TemperatureSensor(const char *name) : sensor(name, value_tag::temperature) { }
	void update() { };
};

class LightSensor : public sensor
{
public:
	LightSensor(const char *name) : sensor(name, value_tag::light) { }
	void update() { };
};

class time_sensor : public sensor
{
public:
	time_sensor(const char *name) : sensor(name, value_tag::time) { }
	void update() {
		auto now = std::chrono::system_clock::now();
		auto now_t = std::chrono::system_clock::to_time_t(now);
		tm tm;
		localtime_s(&tm, &now_t);
		_value.value = tm.tm_hour * 60 + tm.tm_min;
	};
};

}