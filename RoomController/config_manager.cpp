#include "pch.h"
#include "config_manager.h"


config_manager::config_manager(const path & config) : _config(config)
{
}

config_manager::~config_manager()
{
}

void config_manager::set(const wstring& name, const wstring & value)
{
	lock_t lock(_mutex);
	_data.SetNamedValue(name, JsonValue::CreateStringValue(value));
}

void config_manager::set(const wstring& name, long value)
{
	lock_t lock(_mutex);
	_data.SetNamedValue(name, JsonValue::CreateNumberValue(value));
}

wstring config_manager::get(const wstring& name, const wstring& default) const
{
	lock_t lock(_mutex);
	return _data.GetNamedString(name, default);
}

long config_manager::get(const wstring& name, long default) const
{
	lock_t lock(_mutex);
	return (long)_data.GetNamedNumber(name, default);
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
		ofs << wstring(json.ToString());
	} catch(const std::exception&) {
	}
}
