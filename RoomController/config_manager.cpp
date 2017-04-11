#include "pch.h"
#include "config_manager.h"


config_manager::config_manager(const path & config) : _config(config)
{
	load();
}

config_manager::~config_manager()
{
}

void config_manager::set(const wchar_t name[], const wchar_t value[])
{
	lock_t lock(_mutex);
	_data.SetNamedValue(name, JsonValue::CreateStringValue(value));
}

void config_manager::set(const wchar_t name[], int value)
{
	lock_t lock(_mutex);
	_data.SetNamedValue(name, JsonValue::CreateNumberValue(value));
}

void config_manager::set(const wchar_t name[], bool value)
{
	lock_t lock(_mutex);
	_data.SetNamedValue(name, JsonValue::CreateBooleanValue(value));
}

wstring config_manager::get(const wchar_t name[], const wchar_t default[]) const
{
	lock_t lock(_mutex);
	return _data.GetNamedString(name, default);
}

int config_manager::get(const wchar_t name[], int default) const
{
	lock_t lock(_mutex);
	return (int)_data.GetNamedNumber(name, default);
}

bool config_manager::get(const wchar_t name[], bool default) const
{
	lock_t lock(_mutex);
	return _data.GetNamedBoolean(name, default);
}

void config_manager::load()
{
	lock_t lock(_mutex);
	try {
		if(!std::experimental::filesystem::exists(_config))	return;
		std::wifstream ifs(_config);
		std::wstringstream wss;
		ifs.imbue(_utf8_locale);
		wss << ifs.rdbuf();
		auto json = JsonObject::Parse(wss.str());
		_data = json.GetNamedObject(L"config");
	} catch(const std::exception&) {} catch(const winrt::hresult_error&) {}
}

void config_manager::save() const
{
	lock_t lock(_mutex);
	try {
		std::wofstream ofs(_config);
		ofs.imbue(_utf8_locale);
		JsonObject json;
		json.SetNamedValue(L"config", _data);
		ofs << wstring(json.Stringify());
	} catch(const std::exception&) {
	}
}
