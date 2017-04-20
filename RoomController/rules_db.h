#pragma once

using std::wstring;
using std::experimental::filesystem::path;

namespace roomctrl {

enum class rule_status { error, active, inactive };

struct rule {
	rule() : id(0) {}
	rule(unsigned id, const wstring& name, const wstring& condition, const wstring& action) : id(id), name(name), condition(condition), action(action) {}
	rule(const wstring& name, const wstring& condition, const wstring& action) : rule(0, name, condition, action) {}
	wstring to_string() const;

	unsigned id;
	wstring name;
	wstring condition;
	wstring action;
	rule_status	status = rule_status::inactive;
};
using rules_v = std::vector<rule>;

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

}