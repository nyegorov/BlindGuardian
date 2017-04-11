#pragma once

using std::map;
using std::wstring;
using std::experimental::filesystem::path;
using namespace winrt::Windows::Data::Json;

class config_manager
{
	using lock_t = std::lock_guard<std::mutex>;
	mutable	std::mutex	_mutex;
	path				_config;
	JsonObject			_data;
	std::locale			_utf8_locale{ std::locale(), new std::codecvt_utf8<wchar_t>() };

public:
	config_manager(const path& storage);
	~config_manager();
	void set(const wchar_t name[], const wchar_t value[]);
	void set(const wchar_t name[], int value);
	void set(const wchar_t name[], bool value);

	wstring get(const wchar_t name[], const wchar_t default[] = L"") const;
	int get(const wchar_t name[], int default) const;
	bool get(const wchar_t name[], bool default) const;

	void load();
	void save() const;
};

