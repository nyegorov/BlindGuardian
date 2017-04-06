#include "pch.h"
#include "sensors.h"

namespace roomctrl {

void sensor::set(value_t val) { 
	_value = val; 
	_min = std::min((value_t)_min, (value_t)_value);
	_max = std::max((value_t)_max, (value_t)_value);
}

void sensor::reset() { 
	_min = (value_t)_value; 
	_max = (value_t)_value; 
}

}

