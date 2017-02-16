#pragma once

#include "Value.h"

namespace BlindGuardian
{

using std::string;

struct ISensor
{
	virtual string GetName() = 0;
	virtual value_t GetValue() = 0;
	virtual void Update() = 0;
};

struct IActuator
{
	virtual string GetName() = 0;
	virtual void Activate(value_t value) = 0;
};

class SensorBase : public ISensor
{
protected:
	value_t _value;
	std::string _name;
public:
	SensorBase(const char *name, value_tag type) : _name(name), _value{ type, 0 } {}
	value_t GetValue() { return _value; }
	string GetName() { return _name; }
};

class TemperatureSensor : public SensorBase
{
public:
	TemperatureSensor(const char *name) : SensorBase(name, value_tag::temperature) { }
	void Update() { _value.value = 24; };
};

class LightSensor : public SensorBase
{
public:
	LightSensor(const char *name) : SensorBase(name, value_tag::light) { }
	void Update() { _value.value = 42; };
};

}