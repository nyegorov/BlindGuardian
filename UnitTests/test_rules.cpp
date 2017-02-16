#include "stdafx.h"
#include "CppUnitTest.h"

#include "../RoomController/Value.h"
#include "../RoomController/Sensor.h"
#include "../RoomController/Rules.h"
#include "../RoomController/parser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace BlindGuardian;

class DumbSensor : public SensorBase
{
public:
	DumbSensor(const char *name, value_tag tag, value_type val) : SensorBase(name, tag) { _value.value = val; }
	void SetValue(value_type val) { _value.value = val; }
	void Update() {}
};

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {
template<> inline std::wstring ToString<value_t>(const value_t& v)	{ return std::to_wstring(v.value); }
template<> inline std::wstring ToString<int64_t>(const int64_t& v)  { return std::to_wstring(v); }
}
}
}


namespace UnitTests
{		
	TEST_CLASS(TestRules)
	{
	public:
		
		TEST_METHOD(Parsing)
		{
			NScript ns;
			ns.set("myfunc", [](value_t p) {return p+p; });
			ns.set("myvar", value_t{ 42 });
			Assert::AreEqual( 4ll, ns.eval("2*2").value);
			Assert::AreEqual( 1ll, ns.eval("2*(5-3)==4").value);
			Assert::AreEqual( 1ll, ns.eval("(1>2 || 1>=2 || 1<=2 || 1<2) && !(3==4) && (3!=4) ? 1 : 0").value);
			Assert::AreEqual( 0ll, ns.eval("(2<=1 || 1<1 || 1>1 || 1<1) && !(3==3) && (3!=3) ? 1 : 0").value);
			Assert::AreEqual(42ll, ns.eval("x=42; x").value);
			Assert::AreEqual( 1ll, ns.eval("x=1; y=2; x=x+y; y=y-x; x=x*y; x=x/y; x=x-1; x+y").value);
			Assert::AreEqual(48ll, ns.eval("x=3; MyFunc(x)+myVar").value);
			Assert::AreEqual(340ll, ns.eval("x=#5:40#; x").value);
		}
		TEST_METHOD(Sensors)
		{
			DumbSensor ts("temp", value_tag::temperature, 24);
			DumbSensor ls("light", value_tag::light, 2000);
			DumbSensor tm("time", value_tag::time, 340);
			RulesEngine re(
				std::vector<ISensor*>{ &ts, &ls, &tm },
				std::vector<IActuator*>{}
			);
			//re.UpdateRules();
			re.Run();
		}
		TEST_METHOD(Errors)
		{
			NScript ns;
			DumbSensor ts("temp",  value_tag::temperature, 24);
			DumbSensor ls("light", value_tag::light, 2000);
			ns.set("myfunc", [](value_t p) {return p; });
			ns.set("t", [&ts](value_t) {return ts.GetValue(); });
			ns.set("l", [&ls](value_t) {return ls.GetValue(); });
			Assert::AreEqual(value_t{ value_tag::error, (value_type)error_t::name_not_found }, ns.eval("my_func(3)"));
			Assert::AreEqual(value_t{ value_tag::error, (value_type)error_t::type_mismatch }, ns.eval("t + l"));
			Assert::AreEqual(value_t{ value_tag::error, (value_type)error_t::type_mismatch }, ns.eval("t == l"));
			Assert::AreEqual(value_t{ value_tag::error, (value_type)error_t::type_mismatch }, ns.eval("t < l"));
		}
		TEST_METHOD(Rules)
		{
			NScript ns;
			DumbSensor ts("temp",  value_tag::temperature, 24);
			DumbSensor ls("light", value_tag::light, 2000);
			DumbSensor tm("time",  value_tag::time,  340);
			value_t pos = 0;

			ns.set("time",		[&tm](value_t) {return tm.GetValue(); });
			ns.set("tin",		[&ts](value_t) {return ts.GetValue(); });
			ns.set("light",		[&ls](value_t) {return ls.GetValue(); });
			ns.set("set_blind", [&pos](value_t p) {return pos = p, value_t{ 1 }; });
			Assert::AreEqual( 1ll, ns.eval("#5:40# == time").value);
			Assert::AreEqual( 1ll, ns.eval("if(tin > 20) set_blind(66)").value);
			Assert::AreEqual(66ll, pos.value);
			Assert::AreEqual( 1ll, ns.eval("if(tin > 20 && light > 1000) set_blind(99)").value);
			Assert::AreEqual(99ll, pos.value);
			ls.SetValue(500);
			Assert::AreEqual( 0ll, ns.eval("if(tin > 20 && light > 1000) set_blind(24)").value);
			Assert::AreEqual(99ll, pos.value);
			ls.SetValue(1500);
			Assert::AreEqual( 1ll, ns.eval("if(tin > 20 && light > 1000) set_blind(24)").value);
			Assert::AreEqual(24ll, pos.value);
		}

	};
}