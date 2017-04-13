#include "pch.h"
#include "sensors.h"

namespace roomctrl {

void sensor::set(value_t val) { 
	_value = val; 
	if(is_error(_min) || (value_t)_min > (value_t)_value)	_min = (value_t)_value;
	if(is_error(_max) || (value_t)_max < (value_t)_value)	_max = (value_t)_value;
}

void sensor::reset() { 
	_min = (value_t)_value; 
	_max = (value_t)_value; 
}

}

