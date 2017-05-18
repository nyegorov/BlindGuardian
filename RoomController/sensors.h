#pragma once

#include "value.h"

using namespace std::chrono_literals;

namespace roomctrl
{

class action : public i_action
{
	using action_t = std::function<void(const params_t&)>;
	wstring		_name;
	action_t	_action;
public:
	action(wstring_view name, action_t func) : _name(name), _action(func) {}
	wstring name() const override { return _name; }
	void activate(const params_t& params) const override { _action(params); }
	void operator() (const params_t& params) const { activate(params); }
	void operator() () const { activate(params_t{}); }
};

class actuator : public i_actuator
{
	wstring _name;
public:
	actuator(wstring_view name) : _name(name) { }
	wstring name() const override { return _name; };
};

class sensor : public i_sensor
{
protected:
	std::atomic<value_t> _value;
	std::atomic<value_t> _min;
	std::atomic<value_t> _max;
	wstring _name;
public:
	sensor(wstring_view name, value_t value = value_type{ 0 }) : _name(name), _value{ value } { reset(); }
	value_t value() const override { return _value; }
	value_t min() const override { return _min;	}
	value_t max() const override { return _max; }
	void set(value_t val);
	void reset() override;
	void update() override {};
	wstring name() const override { return _name; }
};

class missing_sensor final : public sensor
{
public:
	missing_sensor(wstring_view name) : sensor(name)	{ set(error_t::not_implemented); }
	void update() override { }
};

class time_sensor final : public sensor
{
public:
	time_sensor(wstring_view name) : sensor(name) { update(); }
	void update() override {
		auto now = std::chrono::system_clock::now();
		auto now_t = std::chrono::system_clock::to_time_t(now);
		tm tm;
		localtime_s(&tm, &now_t);
		_value = tm.tm_hour * 60 + tm.tm_min;
	};
};

class hcsr501_sensor final : public sensor
{
public:
	hcsr501_sensor(wstring_view name, int32_t motion_pin);
	void update() override;
private:
	winrt::Windows::Devices::Gpio::GpioPin	_motion_pin{ nullptr };
	std::atomic<std::chrono::system_clock::time_point> _last_activity_time{ std::chrono::system_clock::now() };
};

class tmp75_sensor final : public sensor
{
public:
	enum resolution : uint8_t { res9bit = 1, res10bit, res11bit, res12bit };

	tmp75_sensor(wstring_view name, resolution res, uint32_t address) : sensor(name), _res(res), _address(address) {}
	std::future<void> start();
	void update() override;
private:
	winrt::Windows::Devices::I2c::I2cDevice _tmp75{ nullptr };
	uint32_t	_address;
	resolution	_res;
};

class beeper final : public actuator
{
public:
	beeper(std::wstring_view name, int32_t beeper_pin);
	std::vector<const i_action*> actions() const override { return{ &_beep }; }
	void beep(std::chrono::milliseconds duration);
	void beep() { beep(300ms); }
private:
	action _beep{ L"beep",    [this](auto& params) { auto t = params.empty() ? 300ms : get_arg<std::chrono::milliseconds>(params, 0); beep(t); } };
	winrt::Windows::Devices::Gpio::GpioPin	_beeper_pin{ nullptr };
};

class led final : public actuator
{
public:
	led(std::wstring_view name, int32_t led_pin);
	action on{  L"on",  [this](auto&) { set(true);	} };
	action off{ L"off", [this](auto&) { set(false);	} };
	action invert { L"invert", [this](auto&) { set(!get());	} };
	std::vector<const i_action*> actions() const override { return{ &on, &off }; }
private:
	void set(bool state);
	bool get();
	winrt::Windows::Devices::Gpio::GpioPin	_led_pin{ nullptr };
};

}