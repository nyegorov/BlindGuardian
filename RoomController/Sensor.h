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

template<class T>class action : public IAction
{
	using method_t = void (T::*)(value_t);
	string		_name;
	T*			_parent;
	method_t	_pmethod;
public:
	action(const char *name, T* parent, method_t pmethod) : _name(name), _parent(parent), _pmethod(pmethod) {}
	string name() { return _name; }
	void activate(value_t value) { (_parent->*_pmethod)(value); }
};

class SensorBase : public ISensor
{
protected:
	value_t _value;
	std::string _name;
public:
	SensorBase(const char *name, value_tag type) : _name(name), _value{ type, 0 } {}
	value_t value() const { return _value; }
	string name() const { return _name; }
};

class TemperatureSensor : public SensorBase
{
public:
	TemperatureSensor(const char *name) : SensorBase(name, value_tag::temperature) { }
	void update() { };
};

class LightSensor : public SensorBase
{
public:
	LightSensor(const char *name) : SensorBase(name, value_tag::light) { }
	void update() { };
};

}