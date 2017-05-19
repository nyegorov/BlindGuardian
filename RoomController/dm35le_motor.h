#pragma once

#include "sensors.h"
#include "config_manager.h"
#include "log_manager.h"

namespace roomctrl {

class dm35le_motor final : public i_motor
{
public:
	enum command : byte { cmdUp = 1, cmdDown = 3, cmdStop = 5 };
	const unsigned repeat_count = 10;

	dm35le_motor(int32_t rx_pin, int32_t tx_pin, sensor& position, config_manager& config);
	~dm35le_motor();
	void start() override	{}
	void open() override	{ send_cmd(cmdUp, repeat_count); _position.set(100); }
	void close() override	{ send_cmd(cmdDown, repeat_count); _position.set(0); }
	void stop() override	{ send_cmd(cmdStop, repeat_count); }
	bool pair_remote (std::chrono::milliseconds timeout, std::function<void(int)> progress);
	int get_remote_id()		{ return _remote_id; }

private:
	unsigned long long read_cmd(std::chrono::milliseconds timeout);
	void send_cmd(command code, uint32_t repeat);

	winrt::Windows::Devices::Gpio::GpioPin	_tx_pin{ nullptr };
	winrt::Windows::Devices::Gpio::GpioPin	_rx_pin{ nullptr };
	sensor&				_position;
	config_manager&		_config;
	int					_remote_id;
};

}