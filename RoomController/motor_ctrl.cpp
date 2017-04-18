#include "pch.h"
#include "motor_ctrl.h"

namespace roomctrl {

motor_ctrl::motor_ctrl(std::wstring_view name) : actuator(name)
{
}

motor_ctrl::motor_ctrl(std::wstring_view name, const vec_motors& motors) : actuator(name), _motors(motors)
{
}

motor_ctrl::~motor_ctrl()
{
}

}