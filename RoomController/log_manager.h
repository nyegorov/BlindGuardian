#pragma once

#include "circular_buffer.h"

using std::wstring;
using std::wstring_view;

enum class log_level	{error, message};

struct log_entry
{
	log_level		level;
	std::chrono::time_point<std::chrono::system_clock>		timestamp;
	long long		elapsed;
	const wchar_t	*module;
	wstring			message;
};

wstring to_string(const log_entry&);

class log_manager
{
	void log(log_level level, const wchar_t module[], const wchar_t message[]);
	bool _enable_debug{ false };

	using lock_t = std::lock_guard<std::mutex>;
	mutable	std::mutex	_mutex;
	circular_buffer<log_entry>	_log{ 1000 };
	std::chrono::time_point<std::chrono::high_resolution_clock>	_start_time;
	std::chrono::time_point<std::chrono::system_clock>			_start_time_sys;
public:
	log_manager();
	~log_manager();

	wstring to_string();
	void enable_debug(bool enable) { _enable_debug = enable; }
	void error(const wchar_t module[], const wchar_t message[]);
	void error(const wchar_t module[], const winrt::hresult_error& hr);
	void error(const wchar_t module[], const std::exception& ex);
	void message(const wchar_t module[], const wchar_t message[]);
	template<class ...ARGS> void message(const wchar_t module[], const wchar_t format[], ARGS... args);
};

template<class ...ARGS>
inline void log_manager::message(const wchar_t module[], const wchar_t format[], ARGS ...args)
{
	wchar_t buf[1024];
	swprintf(buf, sizeof(buf) / sizeof(buf[0]), format, args...);
	log(log_level::message, module, buf);
}