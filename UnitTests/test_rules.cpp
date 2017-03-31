#include "stdafx.h"
#include "CppUnitTest.h"

#include "../RoomController/Value.h"
#include "../RoomController/Sensor.h"
#include "../RoomController/Rules.h"
#include "../RoomController/parser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt::Windows::Data::Json;
using namespace BlindGuardian;

class DumbSensor : public sensor
{
public:
	DumbSensor(const char *name,value_type val) : sensor(name, val) { }
	void set(value_type val) { _value = val; _min = std::min(_min, _value); _max = std::max(_max, _value); }
	void update() {}
};

class DumbMotor : public actuator
{
public:
	value_t	_value = { 0 };
	action open{ "open", [this](auto&) { _value = 100; } };
	action close{ "close", [this](auto&) { _value = 0; } };
	action setpos{ "set_pos", [this](auto& v) { _value = v.empty() ? 0 : v.front(); } };
	DumbMotor(const char *name) : actuator(name) { }
	value_t value() { return _value; };
	std::vector<const i_action*> actions() const { return{ &open, &close, &setpos }; }
};

wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.from_bytes(str);
}

string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.to_bytes(wstr);
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
			DumbSensor ts("temp", 24);
			NScript ns;
			ns.set("myfunc0", [](auto& p) {return 10; });
			ns.set("myfunc1", [](auto& p) {return p[0] + p[0]; });
			ns.set("myfunc2", [](auto& p) {return p[0]+p[1]; });
			ns.set("myvar", 42);
			ns.set("mysens", &ts);
			Assert::AreEqual( 4, get<int32_t>(ns.eval("2*2")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval("2*(5-3)==4")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval("(1>2 || 1>=2 || 1<=2 || 1<2) && !(3==4) && (3!=4) ? 1 : 0")));
			Assert::AreEqual( 0, get<int32_t>(ns.eval("(2<=1 || 1<1 || 1>1 || 1<1) && !(3==3) && (3!=3) ? 1 : 0")));
			Assert::AreEqual(42, get<int32_t>(ns.eval("x=42; x")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval("x=1; y=2; x=x+y; y=y-x; x=x*y; x=x/y; x=x-1; x+y")));
			Assert::AreEqual(10, get<int32_t>(ns.eval("myfunc0()")));
			Assert::AreEqual(24, get<int32_t>(ns.eval("mysens")));
			Assert::AreEqual(48, get<int32_t>(ns.eval("x=3; MyFunc2(x, myVar)+x")));
			Assert::AreEqual(40, get<int32_t>(ns.eval("x=#5:40#; x-300")));
			Assert::AreEqual(114, get<int32_t>(ns.eval("myVar+mysens+myfunc1(mysens)")));
		}
		TEST_METHOD(Errors)
		{
			DumbSensor ts("temp",  24);
			DumbSensor ls("light", 2000);
			NScript ns;
			ns.set("myfunc", [](auto& p) {return p[0]; });
			ns.set("t", [&]() {return ts.value(); });
			ns.set("l", [&]() {return ls.value(); });
			Assert::AreEqual(value_t{ error_t::name_not_found }, ns.eval("my_func(3)"));
			//Assert::AreEqual(value_t{ error_t::type_mismatch }, ns.eval("t + l"));
			//Assert::AreEqual(value_t{ error_t::type_mismatch }, ns.eval("t == l"));
			//Assert::AreEqual(value_t{ error_t::type_mismatch }, ns.eval("t < l"));
		}
		TEST_METHOD(Rules)
		{
			NScript ns;
			DumbSensor ts("temp",  24);
			DumbSensor ls("light", 2000);
			DumbSensor tm("time",  340);
			value_t pos = 0;

			ns.set("time",	&tm);
			ns.set("tin",	&ts);
			ns.set("light",	&ls);
			ns.set("set_blind", [&pos](auto& p) {return pos = p[0], value_t{ 1 }; });
			Assert::AreEqual( 1, get<int32_t>(ns.eval("#5:40# == time")));
			Assert::AreEqual( 1, get<int32_t>(ns.eval("if(tin > 20) set_blind(66)")));
			Assert::AreEqual(66, get<int32_t>(pos));
			Assert::AreEqual( 1, get<int32_t>(ns.eval("if(tin > 20 && light > 1000) set_blind(99)")));
			Assert::AreEqual(99, get<int32_t>(pos));
			ls.set(500);
			Assert::AreEqual( 0, get<int32_t>(ns.eval("if(tin > 20 && light > 1000) set_blind(24)")));
			Assert::AreEqual(99, get<int32_t>(pos));
			ls.set(1500);
			Assert::AreEqual( 1, get<int32_t>(ns.eval("if(tin > 20 && light > 1000) set_blind(24)")));
			Assert::AreEqual(24, get<int32_t>(pos));
		}

		TEST_METHOD(Sensors)
		{
			DumbSensor ts("temp", 24);
			DumbSensor ls("light", 2000);
			time_sensor tm("time");
			tm.update();
			DumbMotor mot1("mot1"), mot2("mot2");
			RoomEngine re(
				{ &ts, &ls, &tm },
				{ &mot1, &mot2 }
			);
			re.update_rules({
				{ "r1", "temp > 30", "mot1.open()" },
				{ "r2", "temp > 30", "mot2.set_pos(50)" },
				{ "r3", "time - lasttime >= 5 && time - lasttime < 6", "mot1.set_pos(42)" },
			});

			// temperature
			re.eval("lasttime = time");
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			ts.set(35);
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(50, get<int32_t>(mot2.value()));

			// time sensor
			re.eval("lasttime = time - 5");
			re.run();
			Assert::AreEqual(42, get<int32_t>(mot1.value()));
			mot1.close();
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
		}
		TEST_METHOD(Sensitivity)
		{
			DumbSensor ts("temp", 0);
			DumbMotor mot1("mot1"), mot2("mot2");
			RoomEngine re( { &ts }, { &mot1, &mot2 } );
			re.update_rules({
				{ "r1", "temp > 30", "mot1.open()" },
				{ "r2", "temp > 30 & temp.min < 25", "mot2.open(); temp.reset()" },
			});

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
		}
		TEST_METHOD(Json)
		{
			auto rules = R"(
			{
				"rules": [
					{ "name": "r1", "condition" : "temp > 30", "body" : "mot1.open() " },
					{ "name": "r2", "condition" : "temp > 30", "body" : "mot2.set_pos(50) "}
				]
			})";
			DumbSensor ts("temp", 35);
			DumbMotor mot1("mot1"), mot2("mot2");
			RoomEngine re( { &ts }, { &mot1, &mot2 } );
			re.update_rules(rules);
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(50, get<int32_t>(mot2.value()));
			auto s1 = re.get_rules();
			RoomEngine re2({ &ts }, { &mot1, &mot2 });
			re2.update_rules(s1);
			re2.run();
			auto s2 = re2.get_rules();
			Assert::AreEqual(s1, s2);
		}


	};
}

