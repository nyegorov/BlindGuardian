#pragma once

#include <chrono>
#include <string>
#include "winrt\base.h"

inline long long gettime() {
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
	return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

inline void log_hresult(const wchar_t *module, const winrt::hresult_error& hr)
{
	wchar_t buf[1024];
	swprintf(buf, _countof(buf), L"% 8lld %s: %s\n", gettime(), module, std::wstring(hr.message()).c_str());
	OutputDebugStringW(buf);
}