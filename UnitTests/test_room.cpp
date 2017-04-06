#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/value.h"
#include "../RoomController/sensors.h"
#include "../RoomController/room_engine.h"
#include "../RoomController/parser.h"
#include "common.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Networking::Sockets;
using namespace std::experimental;
using namespace roomctrl;

void write_rules(path path, const wstring& rules)
{
	std::wofstream ofs(path);
	ofs << rules;
}

void write_rules(path path, const room_server::vec_rules& rules)
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

using std::get;

namespace UnitTests
{		
	TEST_CLASS(RoomEngine)
	{
	public:

		TEST_METHOD(Sensors)
		{
			path p("test.db");
			write_rules(p, {
				{ L"r1", L"temp > 30", L"mot1.open()" },
				{ L"r2", L"temp > 30", L"mot2.set_pos(50)" },
				{ L"r3", L"time - lasttime >= 5 && time - lasttime < 6", L"mot1.set_pos(42)" },
				{ L"r4", L"light > 1000", L"mot3.open()" },
			});
			DumbRemote remote('l', 42);
			DumbSensor ts(L"temp", 24);
			time_sensor tm(L"time");
			udns_resolver udns;
			motor_ctrl motc(L"localhost", udns);
			remote_sensor ls(L"light", 'l', motc);
			DumbMotor mot1(L"mot1"), mot2(L"mot2"), mot3(L"mot3");
			room_server re(p);
			re.init(
				{ &ts, &ls, &tm },
				{ &mot1, &mot2, &mot3 }
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

			// remote
			Assert::AreEqual(0, get<int32_t>(mot3.value()));
			remote.set(2000);
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot3.value()));

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
			room_server re(p);
			re.init( { &ts }, { &mot1, &mot2 } );

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
			room_server re(p1);
			re.init( { &ts }, { &mot1, &mot2 } );
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(50, get<int32_t>(mot2.value()));
			auto s1 = re.get_rules();
			write_rules(p2, s1);
			room_server re2(p2);
			re2.init( { &ts }, { &mot1, &mot2 });
			re2.run();
			auto s2 = re2.get_rules();
			Assert::AreEqual(s1, s2);
			filesystem::remove(p1);
			filesystem::remove(p2);
		}


	};
}

