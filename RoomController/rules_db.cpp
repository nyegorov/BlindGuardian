#include "pch.h"
#include "rules_db.h"

using namespace winrt::Windows::Data::Json;

JsonObject to_json(const rule & rule)
{
	JsonObject json;
	json.SetNamedValue(L"id", JsonValue::CreateNumberValue(rule.id));
	json.SetNamedValue(L"name", JsonValue::CreateStringValue(rule.name));
	json.SetNamedValue(L"condition", JsonValue::CreateStringValue(rule.condition));
	json.SetNamedValue(L"action", JsonValue::CreateStringValue(rule.action));
	json.SetNamedValue(L"status", JsonValue::CreateNumberValue((double)rule.status));
	return json;
}

wstring rule::to_string() const
{
	return to_json(*this).ToString();
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
	JsonObject json;
	json.SetNamedValue(L"rules", jrules);
	return json.Stringify();
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
			auto jo = jr.GetObject();
			_rules.emplace_back(
				(unsigned)jo.GetNamedNumber(L"id"),
				jo.GetNamedString(L"name"),
				jo.GetNamedString(L"condition"),
				jo.GetNamedString(L"action")
			);
		}
	} 
	catch(const std::exception&) {}
	catch(const winrt::hresult_error&) {}
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
			_rules.emplace_back(id, rule.name, rule.condition, rule.action);
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

