#pragma once

#include "value.h"

namespace roomctrl
{

class action : public i_action
{
	wstring		_name;
	std::function<void(const params_t&)> _action;
public:
	action(std::wstring_view name, std::function<void(const params_t&)> func) : _name(name), _action(func) {}
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
	wstring name() const override { return _name; }
};

class missing_sensor : public sensor
{
public:
	missing_sensor(wstring_view name) : sensor(name)	{ set(error_t::not_implemented); }
	void update() override { }
};

class time_sensor : public sensor
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

class hcsr501_sensor : public sensor
{
public:
	hcsr501_sensor(wstring_view name, int32_t motion_pin);
	void update() override;
private:
	//winrt::Windows::System::Threading::ThreadPoolTimer	_timer{ nullptr };
	winrt::Windows::Devices::Gpio::GpioPin	_motion_pin{ nullptr };
	std::atomic<std::chrono::system_clock::time_point> _last_activity_time{ std::chrono::system_clock::now() };
};

class tmp75_sensor : public sensor
{
public:
	enum resolution : uint8_t { res9bit, res10bit, res11bit, res12bit };

	tmp75_sensor(wstring_view name, resolution res) : sensor(name), _res(res) {}
	std::future<void> start();
	void update() override;
private:
	winrt::Windows::Devices::I2c::I2cDevice _tmp75{ nullptr };
	resolution _res;
};

class beeper : public actuator
{
public:
	beeper(std::wstring_view name, int32_t beeper_pin);
	action beep{ L"beep",    [this](auto&) { make_beep(std::chrono::milliseconds(200)); } };
	std::vector<const i_action*> actions() const { return{ &beep }; }
	template<class R, class P> void make_beep(std::chrono::duration<R, P> duration);
private:
	winrt::Windows::Devices::Gpio::GpioPin	_beeper_pin{ nullptr };
};

class led : public actuator
{
public:
	led(std::wstring_view name, int32_t led_pin);
	action on{  L"on",  [this](auto&) { set(true);	} };
	action off{ L"off", [this](auto&) { set(false);	} };
	std::vector<const i_action*> actions() const { return{ &on, &off }; }
private:
	void set(bool state);
	winrt::Windows::Devices::Gpio::GpioPin	_led_pin{ nullptr };
};

}