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
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Web::Http::Headers;
using namespace winrt::Windows::Security::Cryptography;
using namespace std;
using namespace winrt;
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
		json.SetNamedValue(L"enabled", JsonValue::CreateBooleanValue(r.enabled));
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
		TEST_CLASS_INITIALIZE(Init) {
			filesystem::remove("rules.json");
		}
		TEST_CLASS_CLEANUP(Cleanup) {
			filesystem::remove("rules.json");
		}

		TEST_METHOD(Sensors)
		{
			path p(".");
			write_rules(p / "rules.json", {
				{ L"r1", L"temp > 30", L"mot1.open()", true },
				{ L"r2", L"temp > 30", L"mot2.open()", true },
				{ L"r3", L"time - lasttime >= 5 && time - lasttime < 6", L"mot1.open()", true },
				{ L"r4", L"light > 1000", L"mot3.open()", true },
			});
			DumbRemote remote(42, 0);
			DumbSensor ts(L"temp", 24);
			time_sensor tm(L"time");
			sensor temp_out{ L"temp_out", 0 }, light{ L"light", 0 };
			esp8266_sensors	ext{ ESP_PORT, ESP_GROUP, temp_out, light };
			DumbMotor mot1, mot2, mot3;
			motor_ctrl mota{ L"mot1", {&mot1} }, motb{ L"mot2", {&mot2} }, motc{ L"mot3", {&mot3} };
			room_server re(p);
			re.init(
				{ &ts, &tm, &temp_out, &light },
				{ &mota, &motb, &motc }
			);

			// temperature
			re.eval(L"lasttime = time");
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));
			ts.set(35);
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(100, get<int32_t>(mot2.value()));

			// time sensor
			re.eval(L"lasttime = time - 5");
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			mot1.close();
			re.run();
			Assert::AreEqual(0, get<int32_t>(mot1.value()));

			// remote
			Assert::AreEqual(0, get<int32_t>(mot3.value()));
			[&]() -> future<void> {
				co_await winrt::resume_background();
				co_await ext.start();		
				co_await remote.set_light(2000);
				co_await 20ms;
			}().get();

			re.run();
			Assert::AreEqual(100, get<int32_t>(mot3.value()));

			filesystem::remove(p / "rules.json");
		}
		TEST_METHOD(Motors)
		{
			room_server re;
			DumbSensor ts(L"temp", 0);
			DumbMotor mot_a1, mot_a2, mot_b1, mot_b2, mot_b3, mot_c1;

			motor_ctrl mota{ L"mota",{ &mot_a1, &mot_a2 } }, motb{ L"motb",{ &mot_b1, &mot_b2, &mot_b3} }, motc{ L"motc", {&mot_c1} }, motd{ L"motd", {} };
			re.rules().save({ L"o1", L"temp < 30", L"mota.open()", true }, false);
			re.rules().save({ L"o2", L"temp > 20 & temp < 30", L"motc.open(); motd.open()", true }, false);
			re.rules().save({ L"o3", L"temp > 20", L"motb.open()", true }, false);
			re.rules().save({ L"c1", L"temp < 20", L"motb.close(); motc.close(); motd.close()", true }, false);
			re.rules().save({ L"c2", L"temp > 30", L"mota.close(); motc.close(); motd.close()", true }, false);
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
			re.rules().save({ L"r1", L"temp > 30", L"mot1.open()", true }, false);
			re.rules().save({ L"r2", L"temp > 30 & temp.min < 25", L"mot2.open(); temp.reset()", true }, false);
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
					{ "id": 1, "name": "r1", "condition" : "temp > 30", "action" : "mot1.open() ", "enabled" : true },
					{ "id": 2, "name": "r2", "condition" : "temp > 30", "action" : "mot2.open() ", "enabled" : true },
					{ "id": 3, "name": "r3", "condition" : "temp > 30", "action" : "mot3.open() ", "enabled" : false }
				]
			})";
			write_rules(p1 / "rules.json", rules);
			DumbSensor ts(L"temp", 35);
			DumbMotor mot1, mot2, mot3;
			motor_ctrl mota{ L"mot1", {&mot1} }, motb{ L"mot2", { &mot2 } }, motc{ L"mot3",{ &mot3 } };
			room_server re(p1);
			re.init( { &ts }, { &mota, &motb, &motc } );
			re.run();
			Assert::AreEqual(100, get<int32_t>(mot1.value()));
			Assert::AreEqual(100, get<int32_t>(mot2.value()));
			Assert::AreEqual(0, get<int32_t>(mot3.value()));
			filesystem::remove(p1 / "rules.json");
		}
		TEST_METHOD(Tasks)
		{
			path p(L"html");
			filesystem::create_directory(p);
			ofstream ofs(p / "room_status.html"); 
			ofs << "<html><body>OK</body></html>" << endl;
			ofs.close();

			[]() -> future<void> { 
				room_server re(L".");
				co_await winrt::resume_background();
				co_await re.start();
				re.eval(L"blind.close()");
				Assert::AreEqual(0, get<int32_t>(re.eval(L"position")));
				co_await HttpClient().GetAsync({ L"http://localhost/api/set_pos?pos=100" }, HttpCompletionOption::ResponseHeadersRead);
				Assert::AreEqual(0, get<int32_t>(re.eval(L"position")));
				re.run();
				Assert::AreEqual(100, get<int32_t>(re.eval(L"position")));
				co_await HttpClient().GetAsync({ L"http://localhost/api/set_pos?pos=0" }, HttpCompletionOption::ResponseHeadersRead);
				Assert::AreEqual(100, get<int32_t>(re.eval(L"position")));
				re.run();
				Assert::AreEqual(0, get<int32_t>(re.eval(L"position")));
			}().get();
			filesystem::remove_all(p);
		}
		TEST_METHOD(RuleApi) 
		{
			path p(".");
			room_server re(p);
			[&]() -> future<void> {
				co_await winrt::resume_background();
				co_await re.start();
			}().get();

			Assert::AreEqual(0u, re.rules().get_all().size());

			// Create
			rule new_rule{ L"новое правило", L"1&2", L"42", true };
			auto new_id = [&]() -> future<unsigned long> {
				co_await winrt::resume_background();
				auto resp = co_await HttpClient().PostAsync({ L"http://localhost/api/rules" }, HttpStringContent(new_rule.to_string()));
				wstring loc = resp.Headers().Lookup(L"Location");
				if(loc.size() > resp.RequestMessage().RequestUri().Path().size()) {
					return std::stoul(loc.substr(resp.RequestMessage().RequestUri().Path().size() + 1));
				}
				return 0ul;
			}().get();
			auto r2 = re.rules().get(new_id);
			Assert::IsTrue(r2 == new_rule);

			// Read
			auto r3 = [&]() -> future<rule> {
				co_await winrt::resume_background();
				auto resp = co_await HttpClient().GetAsync({ L"http://localhost/api/rules/" + to_wstring(new_id) }, HttpCompletionOption::ResponseContentRead);
				auto result = CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, co_await resp.Content().ReadAsBufferAsync());
				return rule{ JsonObject::Parse(result) };
			}().get();
			Assert::IsTrue(r3 == new_rule);

			// Update
			new_rule.name = L"edited rule";
			[&]() -> future<void> {
				co_await winrt::resume_background();
				auto resp = co_await HttpClient().PutAsync({ L"http://localhost/api/rules/" + to_wstring(new_id) }, HttpStringContent(new_rule.to_string()));
			}().get();
			Assert::IsTrue(re.rules().get(new_id) == new_rule);

			// Delete
			[&]() -> future<void> {
				co_await winrt::resume_background();
				co_await HttpClient().DeleteAsync({ L"http://localhost/api/rules/" + to_wstring(new_id) });
			}().get();

			Assert::AreEqual(0u, re.rules().get_all().size());

			filesystem::remove(p / "rules.json");
		}


	};
}

