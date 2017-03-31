#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/Value.h"
#include "../RoomController/Sensor.h"
#include "../RoomController/room_engine.h"
#include "../RoomController/parser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt::Windows::Data::Json;
using namespace std::experimental;
using namespace roomctrl;

class DumbSensor : public sensor
{
public:
	DumbSensor(const wchar_t *name,value_type val) : sensor(name, val) { }
	void set(value_type val) { _value = val; _min = std::min(_min, _value); _max = std::max(_max, _value); }
	void update() {}
};

class DumbMotor : public actuator
{
public:
	value_t	_value = { 0 };
	action open{ L"open", [this](auto&) { _value = 100; } };
	action close{ L"close", [this](auto&) { _value = 0; } };
	action setpos{ L"set_pos", [this](auto& v) { _value = v.empty() ? 0 : v.front(); } };
	DumbMotor(const wchar_t *name) : actuator(name) { }
	value_t value() { return _value; };
	std::vector<const i_action*> actions() const { return{ &open, &close, &setpos }; }
};

void write_rules(path path, const wstring& rules)
{
	std::wofstream ofs(path);
	ofs << rules;
}

void write_rules(path path, const RoomEngine::vec_rules& rules)
{
	JsonArray jrules;
	unsigned id = 0;
	for (auto& r : rules) {
		JsonObject json;
		json.SetNamedValue(L"id", JsonValue::CreateNumberValue(++id));
		json.SetNamedValue(L"name", JsonValue::CreateStringValue(r.name));
		json.SetNamedValue(L"condition", JsonValue::CreateStringValue(r.condition));
		json.SetNamedValue(L"action", JsonValue::CreateStringValue(r.action));
		json.SetNamedValue(L"status", JsonValue::CreateNumberValue((double)r.status));
		jrules.Append(json);
	}
	JsonObject json;
	json.SetNamedValue(L"rules", jrules);
	return write_rules(path, wstring(json.Stringify()));
}

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {
template<> inline std::wstring ToString<value_t>(const value_t& v) {
	if(auto pv = std::get_if<value_type>(&v))	return std::to_wstring(*pv);
	if(auto pe = std::get_if<error_t>(&v))		return L"error: "s + std::to_wstring((int)*pe);
	return L"unknown type!";
}
}}}

using std::get;

namespace UnitTests
{		
	TEST_CLASS(TestRules)
	{
	public:
		
		TEST_METHOD(Parsing)
		{
			DumbSensor ts(L"temp", 24);
			NScript ns;
			ns.set(L"myfunc0", [](auto& p) {return 10; });
			ns.set(L"myfunc1", [](auto& p) {return p[0] + p[0]; });
			ns.set(L"myfunc2", [](auto& p) {return p[0]+p[1]; });
			ns.set(L"myvar", 42);
			ns.set(L"mysens", &ts);
			Assert::AreEqual( 4, get<int32_t>(ns.eval(L"2*2")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"2*(5-3)==4")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"(1>2 || 1>=2 || 1<=2 || 1<2) && !(3==4) && (3!=4) ? 1 : 0")));
			Assert::AreEqual( 0, get<int32_t>(ns.eval(L"(2<=1 || 1<1 || 1>1 || 1<1) && !(3==3) && (3!=3) ? 1 : 0")));
			Assert::AreEqual(42, get<int32_t>(ns.eval(L"x=42; x")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"x=1; y=2; x=x+y; y=y-x; x=x*y; x=x/y; x=x-1; x+y")));
			Assert::AreEqual(10, get<int32_t>(ns.eval(L"myfunc0()")));
			Assert::AreEqual(24, get<int32_t>(ns.eval(L"mysens")));
			Assert::AreEqual(48, get<int32_t>(ns.eval(L"x=3; MyFunc2(x, myVar)+x")));
			Assert::AreEqual(40, get<int32_t>(ns.eval(L"x=#5:40#; x-300")));
			Assert::AreEqual(114, get<int32_t>(ns.eval(L"myVar+mysens+myfunc1(mysens)")));
		}
		TEST_METHOD(Errors)
		{
			DumbSensor ts(L"temp",  24);
			DumbSensor ls(L"light", 2000);
			NScript ns;
			ns.set(L"myfunc", [](auto& p) {return p[0]; });
			ns.set(L"t", [&]() {return ts.value(); });
			ns.set(L"l", [&]() {return ls.value(); });
			Assert::AreEqual(value_t{ error_t::name_not_found }, ns.eval(L"my_func(3)"));
			//Assert::AreEqual(value_t{ error_t::type_mismatch }, ns.eval(L"t + l"));
			//Assert::AreEqual(value_t{ error_t::type_mismatch }, ns.eval(L"t == l"));
			//Assert::AreEqual(value_t{ error_t::type_mismatch }, ns.eval(L"t < l"));
		}
		TEST_METHOD(Rules)
		{
			NScript ns;
			DumbSensor ts(L"temp",  24);
			DumbSensor ls(L"light", 2000);
			DumbSensor tm(L"time",  340);
			value_t pos = 0;

			ns.set(L"time",	&tm);
			ns.set(L"tin",	&ts);
			ns.set(L"light",	&ls);
			ns.set(L"set_blind", [&pos](auto& p) {return pos = p[0], value_t{ 1 }; });
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"#5:40# == time")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"if(tin > 20) set_blind(66)")));
			Assert::AreEqual(66, get<int32_t>(pos));
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"if(tin > 20 && light > 1000) set_blind(99)")));
			Assert::AreEqual(99, get<int32_t>(pos));
			ls.set(500);
			Assert::AreEqual( 0, get<int32_t>(ns.eval(L"if(tin > 20 && light > 1000) set_blind(24)")));
			Assert::AreEqual(99, get<int32_t>(pos));
			ls.set(1500);
			Assert::AreEqual( 1, get<int32_t>(ns.eval(L"if(tin > 20 && light > 1000) set_blind(24)")));
			Assert::AreEqual(24, get<int32_t>(pos));
		}

		TEST_METHOD(Sensors)
		{
			path p("test.db");
			write_rules(p, {
				{ L"r1", L"temp > 30", L"mot1.open()" },
				{ L"r2", L"temp > 30", L"mot2.set_pos(50)" },
				{ L"r3", L"time - lasttime >= 5 && time - lasttime < 6", L"mot1.set_pos(42)" },
			});
			DumbSensor ts(L"temp", 24);
			DumbSensor ls(L"light", 2000);
			time_sensor tm(L"time");
			tm.update();
			DumbMotor mot1(L"mot1"), mot2(L"mot2");
			RoomEngine re(p, 
				{ &ts, &ls, &tm },
				{ &mot1, &mot2 }
			);

			// temperature
			re.eval(L"lasttime = time");
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			ts.set(35);
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(50, get<int32_t>(mot2.value()));

			// time sensor
			re.eval(L"lasttime = time - 5");
			re.run();
			Assert::AreEqual(42, get<int32_t>(mot1.value()));
			mot1.close();
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			filesystem::remove(p);
		}
		TEST_METHOD(Sensitivity)
		{
			path p("test.db");
			write_rules(p, {
				{ L"r1", L"temp > 30", L"mot1.open()" },
				{ L"r2", L"temp > 30 & temp.min < 25", L"mot2.open(); temp.reset()" },
			});

			DumbSensor ts(L"temp", 0);
			DumbMotor mot1(L"mot1"), mot2(L"mot2");
			RoomEngine re(p, { &ts }, { &mot1, &mot2 } );

			// temperature
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			ts.set(35); re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(100, get<int32_t>(mot2.value()));
			mot1.close();
			mot2.close(); re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			Assert::AreEqual(0, get<int32_t>(mot2.value()));
			ts.set(27); re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			Assert::AreEqual(0, get<int32_t>(mot2.value()));
			ts.set(35); re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(0, get<int32_t>(mot2.value()));
			ts.set(20); re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(0, get<int32_t>(mot2.value()));
			ts.set(35); re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(100, get<int32_t>(mot2.value()));
			filesystem::remove(p);
		}
		TEST_METHOD(Json)
		{
			path p1("test1.db"), p2("test2.db");
			auto rules = LR"(
			{
				"rules": [
					{ "id": 1, "name": "r1", "condition" : "temp > 30", "action" : "mot1.open() " },
					{ "id": 2, "name": "r2", "condition" : "temp > 30", "action" : "mot2.set_pos(50) "}
				]
			})";
			write_rules(p1, rules);
			DumbSensor ts(L"temp", 35);
			DumbMotor mot1(L"mot1"), mot2(L"mot2");
			RoomEngine re(p1, { &ts }, { &mot1, &mot2 } );
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(50, get<int32_t>(mot2.value()));
			auto s1 = re.get_rules();
			write_rules(p2, s1);
			RoomEngine re2(p2, { &ts }, { &mot1, &mot2 });
			re2.run();
			auto s2 = re2.get_rules();
			Assert::AreEqual(s1, s2);
			filesystem::remove(p1);
			filesystem::remove(p2);
		}


	};
}

