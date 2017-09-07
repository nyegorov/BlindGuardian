#include "pch.h"
#include "rules_db.h"

using namespace winrt::Windows::Data::Json;

namespace roomctrl {

JsonObject to_json(const rule & rule)
{
	JsonObject json;
	json.SetNamedValue(L"id", JsonValue::CreateNumberValue(rule.id));
	json.SetNamedValue(L"name", JsonValue::CreateStringValue(rule.name));
	json.SetNamedValue(L"condition", JsonValue::CreateStringValue(rule.condition));
	json.SetNamedValue(L"action", JsonValue::CreateStringValue(rule.action));
	json.SetNamedValue(L"enabled", JsonValue::CreateBooleanValue(rule.enabled));
	json.SetNamedValue(L"status", JsonValue::CreateNumberValue(static_cast<double>(rule.status)));
	return json;
}

rule& rule::operator=(JsonObject json)
{
	for(const auto& kv : json) {
		auto key = kv.Key();
		auto val = kv.Value();
		if(key == L"id")			id = static_cast<unsigned>(val.GetNumber());
		else if(key == L"name")		name = val.GetString();
		else if(key == L"condition")condition = val.GetString();
		else if(key == L"action")	action = val.GetString();
		else if(key == L"enabled")	enabled = val.GetBoolean();
	}
	return *this;
}

wstring rule::to_string() const
{
	auto hs = to_json(*this).ToString();
	return { hs.begin(), hs.end() };
}

wstring to_string(const rules_v&& rules)
{
	JsonArray jrules;
	for(auto& r : rules) jrules.Append(to_json(r));
	auto hs = jrules.Stringify();
	return { hs.begin(), hs.end() };
}

bool operator == (const rule& r1, const rule& r2) {
	return r1.name == r2.name && r1.condition == r2.condition && r1.action == r2.action && r1.enabled == r2.enabled;
}

rules_db::rules_db(const path& storage) : _storage(storage)
{
	load();
}

rules_db::~rules_db()
{
}

wstring rules_db::to_string() const
{
	JsonArray jrules;
	{
		lock_t lock(_mutex);
		for(auto& r : _rules) jrules.Append(to_json(r));
	}
	auto hs = jrules.Stringify();
	return { hs.begin(), hs.end() };
}

void rules_db::load()
{
	try {
		if(!std::experimental::filesystem::exists(_storage))	return;
		std::wifstream ifs(_storage);
		std::wstringstream wss;
		ifs.imbue(_utf8_locale);
		wss << ifs.rdbuf();
		auto json = JsonObject::Parse(wss.str());
		auto jrules = json.GetNamedArray(L"rules");
		for(auto jr : jrules) {
			_rules.emplace_back(jr.GetObject());
		}
	} catch(const std::exception&) {} catch(const winrt::hresult_error&) {}
}

void rules_db::store() const
{
	try {
		std::wofstream ofs(_storage);
		ofs.imbue(_utf8_locale);
		ofs << L"{\"rules\":[";
		for(unsigned i = 0; i < _rules.size(); i++) ofs << (i ? "," : "") << _rules[i].to_string();
		ofs << L"]}";
	} catch(const std::exception&) {

	}
}

rules_v rules_db::get_all() const
{
	lock_t lock(_mutex);
	rules_v res = _rules;
		return res;
}

std::vector<unsigned> rules_db::get_ids() const {
	std::vector<unsigned> ids;
	lock_t lock(_mutex);
	std::transform(begin(_rules), end(_rules), std::back_inserter(ids), [](auto&& r) {return r.id; });
	return ids;
}

rule rules_db::get(unsigned id) const
{
	lock_t lock(_mutex);
	auto it = std::find_if(begin(_rules), end(_rules), [id](auto&&r) {return r.id == id; });
	return it == end(_rules) ? rule() : *it;
}

void rules_db::set_status(unsigned id, rule_status status)
{
	lock_t lock(_mutex);
	auto it = std::find_if(begin(_rules), end(_rules), [id](auto&&r) {return r.id == id; });
	if(it != end(_rules)) it->status = status;
}

unsigned rules_db::save(const rule & rule, bool store_db)
{
	unsigned id = 0;
	{
		lock_t lock(_mutex);
		auto it = std::find_if(begin(_rules), end(_rules), [id = rule.id](auto&&r) {return r.id == id; });
		if(rule.id != 0 && it != _rules.end()) {
			id = rule.id;
			*it = rule;
		} else {
			id = std::accumulate(begin(_rules), end(_rules), 0u, [](auto id, auto& r) {return std::max(r.id, id); }) + 1;
			_rules.emplace_back(id, rule.name, rule.condition, rule.action, rule.enabled);
		}
	}
	if(store_db)	store();
	return id;
}

void rules_db::remove(unsigned id)
{
	lock_t lock(_mutex);
	_rules.erase(std::remove_if(begin(_rules), end(_rules), [id](auto&&r) {return r.id == id; }), end(_rules));
	store();
}

}