#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/parser.h"
#include "../RoomController/rules_db.h"
#include "../RoomController/config_manager.h"
#include "../RoomController/http_server.h"
#include "../RoomController/debug_stream.h"
#include "../RoomController/udns_resolver.h"
#include "../RoomController/motor_ctrl.h"
#include "../RoomController/esp8266_motor.h"
#include "common.h"

using namespace std;
using namespace std::experimental;
using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Storage::Streams;
using namespace roomctrl;

wdebugstream wdebug;
debugstream debug;
log_manager logger;

namespace UnitTests
{		
	TEST_MODULE_INITIALIZE(InitTest)
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
		logger.enable_debug(true);
	}
	TEST_CLASS(Components)
	{
	public:
		TEST_METHOD(Parser)
		{
			DumbSensor ts(L"temp", 24);
			NScript ns;
			ns.set(L"myfunc0", [](auto& p) {return 10; });
			ns.set(L"myfunc1", [](auto& p) {return p[0] + p[0]; });
			ns.set(L"myfunc2", [](auto& p) {return p[0] + p[1]; });
			ns.set(L"myvar", 42);
			ns.set(L"mysens", &ts);
			ns.set(L"x", 3);
			Assert::AreEqual(4, get<int32_t>(ns.eval(L"2*2")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"2*(5-3)==4")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"(1>2 || 1>=2 or 1<=2 || 1<2) and !(3==4) && (3!=4) ? 1 : 0")));
			Assert::AreEqual(0, get<int32_t>(ns.eval(L"(2<=1 || 1<1 || 1>1 || 1<1) and not (3==3) && (3!=3) ? 1 : 0")));
			Assert::AreEqual(42, get<int32_t>(ns.eval(L"x=42; x")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"x=1; y=2; x=x+y; y=y-x; x=x*y; x=x/y; x=x-1; x+y")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"x=7; y=3; x % y")));
			Assert::AreEqual(10, get<int32_t>(ns.eval(L"myfunc0()")));
			Assert::AreEqual(24, get<int32_t>(ns.eval(L"mysens")));
			Assert::AreEqual(40, get<int32_t>(ns.eval(L"x=#5:40#; x-300")));
			Assert::AreEqual(40, get<int32_t>(ns.eval(L"x=5:40; x-300")));
			Assert::AreEqual(48, get<int32_t>(ns.eval(L"x=3; MyFunc2(x, myVar)+x")));
			Assert::AreEqual(114, get<int32_t>(ns.eval(L"myVar+mysens+myfunc1(mysens)")));
			Assert::AreEqual(0, get<int32_t>(ns.eval(L"x=4", true)));
			Assert::AreEqual(3, get<int32_t>(ns.eval(L"x")));
			Assert::AreEqual(0, get<int32_t>(ns.eval(L"if(x=4) 1 else 0")));
			Assert::AreEqual(3, get<int32_t>(ns.eval(L"x")));
			Assert::AreEqual(0, get<int32_t>(ns.eval(L"(x = 4) ? 1 : 0")));
			Assert::AreEqual(3, get<int32_t>(ns.eval(L"x")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"x = 4 ? 1 : 0")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"x")));
		}
		TEST_METHOD(ParserErrors)
		{
			DumbSensor ts(L"temp", 24);
			DumbSensor ls(L"light", 2000);
			missing_sensor ms(L"err");
			DumbRemote remote('t', 42);

			NScript ns;
			ns.set(L"temp", &ts);
			ns.set(L"myfunc", [](auto& p) {return p[0]; });
			ns.set(L"t", [&]() {return ts.value(); });
			ns.set(L"l", [&]() {return ls.value(); });
			ns.set(L"e", [&]() {return ms.value(); });
			Assert::AreEqual(value_t{ error_t::name_not_found }, ns.eval(L"my_func(3)"));
			Assert::AreEqual(value_t{ error_t::name_not_found }, ns.eval(L"my_var"));
			Assert::AreEqual(value_t{ error_t::runtime }, ns.eval(L"temp=3"));
			Assert::AreEqual(value_t{ error_t::not_implemented }, ns.eval(L"e"));
			Assert::AreEqual(value_t{ error_t::not_implemented }, ns.eval(L"e+l"));
			Assert::AreEqual(value_t{ error_t::not_implemented }, ns.eval(L"t+e"));
		}
		TEST_METHOD(RulesExecution)
		{
			NScript ns;
			DumbSensor ts(L"temp", 24);
			DumbSensor ls(L"light", 2000);
			DumbSensor tm(L"time", 340);
			value_t pos = 0;

			ns.set(L"time", &tm);
			ns.set(L"tin", &ts);
			ns.set(L"light", &ls);
			ns.set(L"set_blind", [&pos](auto& p) {return pos = p[0], value_t{ 1 }; });
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"#5:40# == time")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"5:40 = time", true)));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"if(tin > 20) set_blind(66)")));
			Assert::AreEqual(66, get<int32_t>(pos));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"if(tin > 20 && light > 1000) set_blind(99)")));
			Assert::AreEqual(99, get<int32_t>(pos));
			ls.set(500);
			Assert::AreEqual(0, get<int32_t>(ns.eval(L"if(tin > 20 && light > 1000) set_blind(24)")));
			Assert::AreEqual(99, get<int32_t>(pos));
			ls.set(1500);
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"if(tin > 20 && light > 1000) set_blind(24)")));
			Assert::AreEqual(24, get<int32_t>(pos));
		}

		TEST_METHOD(ConfigDatabase)
		{
			path p("config.db");
			filesystem::remove(p);
			config_manager cm(p);
			cm.set(L"nkey", 42l);
			cm.set(L"skey", L"value");
			cm.set(L"bkey", true);
			Assert::AreEqual(42, cm.get(L"nkey", 0));
			Assert::AreEqual(L"value"s, cm.get(L"skey", L""));
			Assert::IsTrue(cm.get(L"bkey", false));
			Assert::IsFalse(cm.get(L"bkey2", false));
			cm.set(L"nkey", -42);
			cm.set(L"skey", L"another");
			Assert::AreEqual(-42, cm.get(L"nkey", 0));
			Assert::AreEqual(L"another"s, cm.get(L"skey", L""));
			cm.save();
			config_manager cm1(p);
			cm1.load();
			Assert::AreEqual(-42, cm.get(L"nkey", 0));
			Assert::AreEqual(L"another"s, cm.get(L"skey", L""));
			filesystem::remove(p);
		}
		TEST_METHOD(RulesDatabase)
		{
			path p("test.db");
			filesystem::remove(p);
			rules_db db(p);
			unsigned id;
			Assert::AreEqual(0u, db.get_all().size());
			id = db.save({ L"r1", L"tin > 30", L"close_blind" });
			Assert::AreEqual(1u, id);
			id = db.save({ L"r2", L"tin < 20", L"open_blind" });
			Assert::AreEqual(2u, id);
			Assert::AreEqual(2u, db.get_all().size());
			rules_db db2(p);
			Assert::AreEqual(2u, db2.get_all().size());
			Assert::AreEqual(L"r1"s, db.get(1).name);
			Assert::AreEqual(L"r2"s, db.get(2).name);
			auto r = db.get(2);
			r.name = L"r2a";
			r.status = rule_status::active;
			id = db.save(r);
			db.set_status(id, rule_status::error);
			Assert::AreEqual(2u, id);
			Assert::AreEqual(L"r2a"s, db.get(id).name);
			db.remove(1);
			Assert::AreEqual(1u, db.get_all().size());
			id = db.get_all()[0].id;
			Assert::AreEqual(2u, id);
			Assert::AreEqual(L"r2a"s, db.get(id).name);
			auto json = r.to_string();
			Assert::AreEqual(L"{\"id\":2,\"name\":\"r2a\",\"condition\":\"tin < 20\",\"action\":\"open_blind\",\"status\":1}"s, json);
			Assert::AreEqual((int)rule_status::error, (int)db.get(id).status);
			db.remove(id);
			Assert::AreEqual(0u, db.get_all().size());
			filesystem::remove(p);
		}

		TEST_METHOD(HttpServer)
		{
			path p(L"test.jpg");
			ofstream ofs(p, ios::binary);
			string test("abc\0\1\2пнх", 9);
			ofs.write(test.c_str(), test.size());
			ofs.close();
			http_server srv(L"666", L"unit test server" );
			wstring value;
			srv.on(L"/",	 [](auto&&, auto&&) { return std::make_tuple(content_type::html, L"MAIN"); });
			srv.on(L"/test", [](auto&&, auto&&) { return std::make_tuple(content_type::html, L"OK"); });
			srv.on(L"/some", [](auto&&, auto&&) { return std::make_tuple(content_type::html, L"YEP"); });
			srv.on(L"/file", p);
			srv.on_action(L"doit", [&value](auto&&, auto&& v) { value = v; });
			srv.start().get();

			wstring content;
			content = async([]() { return HttpClient().GetStringAsync({ L"http://localhost:666" }).get(); }).get();
			Assert::AreEqual(L"MAIN"s, content);
			content = async([]() { return HttpClient().GetStringAsync({ L"http://localhost:666/test" }).get(); }).get();
			Assert::AreEqual(L"OK"s, content);
			content = async([]() { return HttpClient().GetStringAsync({ L"http://localhost:666/some?doit=YES" }).get(); }).get();
			Assert::AreEqual(L"YEP"s, content);
			Assert::AreEqual(L"YES"s, value);

			IBuffer buf;
			array<uint8_t, 9> res;
			buf = async([]() { return HttpClient().GetBufferAsync({ L"http://localhost:666/file" }).get(); }).get();
			DataReader::FromBuffer(buf).ReadBytes(res);
			Assert::AreEqual(0, memcmp(res.data(), test.data(), res.size()));
			filesystem::remove(p);
		}

		TEST_METHOD(Logger)
		{
			auto test = L"TEST";
			path p(L"test.log");
			filesystem::remove(p);
			log_manager logger{ p };
			logger.enable_debug(true);
			logger.error(test, winrt::hresult_error(E_ACCESSDENIED));
			logger.error(test, std::runtime_error("runtime error"));
			logger.error(test, L"another error");
			logger.message(test, L"message");
			logger.message(test, L"%s: %d", L"Question of Life", 42);
			logger.info(test, L"information");
			logger.info(test, L"information: PI=%lf", atan2(1., 1.) * 4.);
			auto jo = JsonObject::Parse(logger.to_string());
			auto je = jo.GetNamedArray(L"entries");
			Assert::AreEqual(to_str(std::this_thread::get_id()), wstring(je.GetObjectAt(0).GetNamedString(L"thread")));
			Assert::AreEqual(0., je.GetObjectAt(6).GetNamedNumber(L"level"));
			Assert::AreEqual(1., je.GetObjectAt(0).GetNamedNumber(L"level"));
			Assert::AreEqual(2., je.GetObjectAt(3).GetNamedNumber(L"level"));
			Assert::AreEqual(L"TEST"s, wstring(je.GetObjectAt(0).GetNamedString(L"module")));
			Assert::AreEqual(L"information: PI=3.141593"s, wstring(je.GetObjectAt(0).GetNamedString(L"message")));
			Assert::AreEqual(L"information"s, wstring(je.GetObjectAt(1).GetNamedString(L"message")));
			Assert::AreEqual(L"Question of Life: 42"s, wstring(je.GetObjectAt(2).GetNamedString(L"message")));
			Assert::AreEqual(L"message"s, wstring(je.GetObjectAt(3).GetNamedString(L"message")));
			Assert::AreEqual(L"another error"s, wstring(je.GetObjectAt(4).GetNamedString(L"message")));
			Assert::AreEqual(L"runtime error"s, wstring(je.GetObjectAt(5).GetNamedString(L"message")));
			Assert::AreEqual(L"0x80070005: Отказано в доступе."s, wstring(je.GetObjectAt(6).GetNamedString(L"message")));
			logger.dump();
			Assert::IsTrue(filesystem::exists(p));
		}

	};
}