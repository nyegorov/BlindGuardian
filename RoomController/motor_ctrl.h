#pragma once

#include "value.h"
#include "sensors.h"
#include "log_manager.h"
#include "udns_resolver.h"
#include "config_manager.h"

using std::wstring;
using namespace std::placeholders;
using namespace std::chrono_literals;

namespace roomctrl {

class motor_ctrl final : public actuator
{
public:
	using vec_motors = std::vector<i_motor*>;

	motor_ctrl(std::wstring_view name);
	motor_ctrl(std::wstring_view name, const vec_motors& motors);
	~motor_ctrl();

	void start() { for(auto m : _motors) m->start(); };
	action open{  L"open",    [this](auto&)	{ do_action(std::mem_fn(&i_motor::open)); } };
	action stop{  L"stop",    [this](auto&) { do_action(std::mem_fn(&i_motor::stop)); } };
	action close{ L"close",   [this](auto&)	{ do_action(std::mem_fn(&i_motor::close)); } };
	std::vector<const i_action*> actions() const override { return{ &open, &close, &stop }; }
private:
	template<class T> void do_action(T t);
	vec_motors	_motors;
};

template<class T> void motor_ctrl::do_action(T action)
{
	for(auto m : _motors) action(m);
	/*std::vector<std::future<void>>	results;
	for(auto m : _motors) {
		results.push_back(std::async(std::launch::async, action, m));
	}
	for(auto& f : results)	f.get();*/
}

}