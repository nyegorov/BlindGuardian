#include "pch.h"
#include "log_manager.h"

using namespace winrt::Windows::Data::Json;

wstring to_string(const log_entry& entry)
{
	auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
	tm tm;
	localtime_s(&tm, &time_t);

	wchar_t buf[1024];
	auto level = entry.level == log_level::error ? L"ERR" : 
		entry.level == log_level::info ? L"INF" : L"MSG";
	swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"% 2d:%02d:%02d (%.3lf) [%s.%s] %s", tm.tm_hour, tm.tm_min, tm.tm_sec, entry.elapsed / 1000., entry.module, level, entry.message.c_str());
	return wstring(buf);
}

log_manager::log_manager()
{
	_start_time = std::chrono::high_resolution_clock::now();
	_start_time_sys = std::chrono::system_clock::now();
}

log_manager::log_manager(path_t path) : _path(path)
{
	_start_time = std::chrono::high_resolution_clock::now();
	_start_time_sys = std::chrono::system_clock::now();
	std::experimental::filesystem::remove(path);
}

log_manager::~log_manager()
{
}

void log_manager::log(log_level level, const wchar_t module[], const wchar_t message[])
{
	lock_t lock(_mutex);
	_log.push_back({
		level,
		std::chrono::system_clock::now(),
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _start_time).count(),
		module, 
		message
	});
	if(_enable_debug) {
		OutputDebugStringW(::to_string(_log.back()).c_str());
		OutputDebugStringW(L"\n");
		if(!_path.empty() && _log.size() == _log.capacity()) {
			dump();
		}
	}
}

void log_manager::dump() {
	try {
		std::wofstream ofs(_path, std::ios::app);
		for(auto& e : _log) ofs << ::to_string(e) << std::endl;
	} catch(const std::exception&) { }
	_log.clear();
}

void log_manager::error(const wchar_t module[], const wchar_t message[])
{
	log(log_level::error, module, message);
}

void log_manager::error(const wchar_t module[], const winrt::hresult_error & hr)
{
	wchar_t buf[1024];
	swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"0x%08x: %ls", hr.code(), hr.message().c_str());
	log(log_level::error, module, buf);
}

void log_manager::error(const wchar_t module[], const std::exception & ex)
{
	wchar_t buf[1024];
	swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%hs", ex.what());
	log(log_level::error, module, buf);
}

void log_manager::message(const wchar_t module[], const wchar_t message[])
{
	log(log_level::message, module, message);
}

void log_manager::info(const wchar_t module[], const wchar_t message[])
{
	log(log_level::info, module, message);
}

template <typename T> struct reversion_wrapper { T& iterable; };
template <typename T> auto begin(reversion_wrapper<T> w) { return rbegin(w.iterable); }
template <typename T> auto end(reversion_wrapper<T> w) { return rend(w.iterable); }
template <typename T> reversion_wrapper<T> reverse(T&& iterable) { return { iterable }; }

wstring log_manager::to_string()
{
	lock_t lock(_mutex);
	JsonArray jentries;
	wchar_t buf[256];
	for(auto& e : reverse(_log)) {
		JsonObject je;
		auto time_t = std::chrono::system_clock::to_time_t(e.timestamp);
		tm tm;
		localtime_s(&tm, &time_t);
		swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
		je.SetNamedValue(L"level", JsonValue::CreateNumberValue((double)e.level));
		je.SetNamedValue(L"timestamp", JsonValue::CreateStringValue(buf));
		je.SetNamedValue(L"elapsed", JsonValue::CreateNumberValue(e.elapsed / 1.));
		je.SetNamedValue(L"module", JsonValue::CreateStringValue(e.module));
		je.SetNamedValue(L"message", JsonValue::CreateStringValue(e.message));
		jentries.Append(je);
	}
	auto time_t = std::chrono::system_clock::to_time_t(_start_time_sys);
	tm tm;
	localtime_s(&tm, &time_t);
	swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%d.%02d.%04d %02d:%02d:%02d", tm.tm_mday, tm.tm_mon, 1900 + tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
	JsonObject json;
	json.SetNamedValue(L"start", JsonValue::CreateStringValue(buf));
	json.SetNamedValue(L"entries", jentries);
	return json.ToString();
}
