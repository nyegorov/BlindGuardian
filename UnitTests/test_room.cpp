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
			path p(".");
			write_rules(p / "rules.json", {
				{ L"r1", L"temp > 30", L"mot1.open()" },
				{ L"r2", L"temp > 30", L"mot2.set_pos(50)" },
				{ L"r3", L"time - lasttime >= 5 && time - lasttime < 6", L"mot1.set_pos(42)" },
				{ L"r4", L"light > 1000", L"mot3.open()" },
			});
			DumbRemote remote(42, 0);
			DumbSensor ts(L"temp", 24);
			time_sensor tm(L"time");
			udns_resolver udns;
			esp8266_motor mote(L"localhost", udns );
			DumbMotor mot1, mot2, mot3;
			motor_ctrl mota{ L"mot1", {&mot1} }, motb{ L"mot2", {&mot2} }, motc{ L"mot3", {&mot3} };
			room_server re(p);
			re.init(
				{ &ts, mote.get_temp(), mote.get_light(), &tm },
				{ &mota, &motb, &motc }
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
			remote.set_light(2000);
			re.run();
			Sleep(100);
			re.run();
			Sleep(100);
			Assert::AreEqual(100, get<int32_t>(mot3.value()));

			filesystem::remove(p / "rules.json");
		}
		TEST_METHOD(Motors)
		{
			room_server re;
			DumbSensor ts(L"temp", 0);
			DumbMotor mot_a1, mot_a2, mot_b1, mot_b2, mot_b3, mot_c1;

			motor_ctrl mota{ L"mota",{ &mot_a1, &mot_a2 } }, motb{ L"motb",{ &mot_b1, &mot_b2, &mot_b3} }, motc{ L"motc", {&mot_c1} }, motd{ L"motd", {} };
			re.rules().save({ L"o1", L"temp < 30", L"mota.open()" }, false);
			re.rules().save({ L"o2", L"temp > 20 & temp < 30", L"motc.open(); motd.open()" }, false);
			re.rules().save({ L"o3", L"temp > 20", L"motb.open()" }, false);
			re.rules().save({ L"c1", L"temp < 20", L"motb.close(); motc.close(); motd.close()" }, false);
			re.rules().save({ L"c2", L"temp > 30", L"mota.close(); motc.close(); motd.close()" }, false);
			re.init({ &ts }, { &mota, &motb, &motc, &motd });
			ts.set(0); re.run();
			Assert::AreEqual(200, mot_a1.pos() + mot_a2.pos());
			Assert::AreEqual(  0, mot_b1.pos() + mot_b2.pos() + mot_b3.pos());
			Assert::AreEqual(  0, mot_c1.pos());
			ts.set(25); re.run();
			Assert::AreEqual(200, mot_a1.pos() + mot_a2.pos());
			Assert::AreEqual(300, mot_b1.pos() + mot_b2.pos() + mot_b3.pos());
			Assert::AreEqual(100, mot_c1.pos());
			ts.set(40); re.run();
			Assert::AreEqual(  0, mot_a1.pos() + mot_a2.pos());
			Assert::AreEqual(300, mot_b1.pos() + mot_b2.pos() + mot_b3.pos());
			Assert::AreEqual(  0, mot_c1.pos());
		}
		TEST_METHOD(Sensitivity)
		{
			DumbSensor ts(L"temp", 0);
			DumbMotor mot1, mot2;
			motor_ctrl mota{ L"mot1", {&mot1} }, motb{ L"mot2",{ &mot2 } };
			room_server re(L".");
			re.rules().save({ L"r1", L"temp > 30", L"mot1.open()" }, false);
			re.rules().save({ L"r2", L"temp > 30 & temp.min < 25", L"mot2.open(); temp.reset()" }, false);
			re.init( { &ts }, { &mota, &motb } );

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
			path p1(".");
			auto rules = LR"(
			{
				"rules": [
					{ "id": 1, "name": "r1", "condition" : "temp > 30", "action" : "mot1.open() " },
					{ "id": 2, "name": "r2", "condition" : "temp > 30", "action" : "mot2.set_pos(50) "}
				]
			})";
			write_rules(p1 / "rules.json", rules);
			DumbSensor ts(L"temp", 35);
			DumbMotor mot1, mot2;
			motor_ctrl mota{ L"mot1", {&mot1} }, motb{ L"mot2", { &mot2 } };
			room_server re(p1);
			re.init( { &ts }, { &mota, &motb } );
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(50, get<int32_t>(mot2.value()));
			auto s1 = re.get_rules();
			write_rules(p1 / "rules.json", s1);
			room_server re2(p1);
			re2.init( { &ts }, { &mota, &motb });
			re2.run();
			auto s2 = re2.get_rules();
			Assert::AreEqual(s1, s2);
			filesystem::remove(p1 / "rules.json");
		}


	};
}

