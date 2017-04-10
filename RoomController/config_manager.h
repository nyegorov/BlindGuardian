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
	void set(const wstring& name, const wstring& value);
	void set(const wstring& name, long value);

	wstring get(const wstring& name, const wstring&  default = L"") const;
	long get(const wstring& name, long default) const;

	void load();
	void save() const;
};

