#pragma once

using std::wstring;
using std::experimental::filesystem::path;

namespace roomctrl {

enum class rule_status { error, active, inactive};

struct rule {
	rule() : id(0) {}
	rule(unsigned id, const wstring& name, const wstring& condition, const wstring& action, bool enabled) : id(id), name(name), condition(condition), action(action), enabled(enabled) {}
	rule(const wstring& name, const wstring& condition, const wstring& action, bool enabled) : rule(0, name, condition, action, enabled) {}
	rule(winrt::Windows::Data::Json::JsonObject json) { *this = json; }
	rule& operator = (winrt::Windows::Data::Json::JsonObject json);
	wstring to_string() const;

	unsigned id = 0;
	wstring name;
	wstring condition;
	wstring action;
	bool enabled;
	rule_status	status = rule_status::inactive;
};
using rules_v = std::vector<rule>;

bool operator == (const rule& r1, const rule& r2);

class rules_db
{
	void store() const;
	void load();

	using lock_t = std::lock_guard<std::mutex>;
	mutable std::mutex	_mutex;
	rules_v				_rules;
	std::locale			_utf8_locale{ std::locale(), new std::codecvt_utf8<wchar_t>() };
	path				_storage;
public:
	using model = rule;
	rules_db(const path& storage);
	~rules_db();

	wstring to_string() const;
	rules_v get_all() const;
	std::vector<unsigned> get_ids() const;
	rule get(unsigned id) const;
	void set_status(unsigned id, rule_status status);
	unsigned save(const rule& rule, bool store_db = true);
	void remove(unsigned id);
};

wstring to_string(const rules_v&& rules);

}