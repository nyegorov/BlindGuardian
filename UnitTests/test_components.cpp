#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/parser.h"
#include "../RoomController/rules_db.h"
#include "../RoomController/config_manager.h"
#include "../RoomController/http_server.h"
#include "../RoomController/debug_stream.h"
#include "../RoomController/udns_resolver.h"
#include "../RoomController/motor_ctrl.h"
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

namespace UnitTests
{		
	TEST_MODULE_INITIALIZE(InitTest)
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
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
			Assert::AreEqual(4, get<int32_t>(ns.eval(L"2*2")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"2*(5-3)==4")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"(1>2 || 1>=2 || 1<=2 || 1<2) && !(3==4) && (3!=4) ? 1 : 0")));
			Assert::AreEqual(0, get<int32_t>(ns.eval(L"(2<=1 || 1<1 || 1>1 || 1<1) && !(3==3) && (3!=3) ? 1 : 0")));
			Assert::AreEqual(42, get<int32_t>(ns.eval(L"x=42; x")));
			Assert::AreEqual(1, get<int32_t>(ns.eval(L"x=1; y=2; x=x+y; y=y-x; x=x*y; x=x/y; x=x-1; x+y")));
			Assert::AreEqual(10, get<int32_t>(ns.eval(L"myfunc0()")));
			Assert::AreEqual(24, get<int32_t>(ns.eval(L"mysens")));
			Assert::AreEqual(48, get<int32_t>(ns.eval(L"x=3; MyFunc2(x, myVar)+x")));
			Assert::AreEqual(40, get<int32_t>(ns.eval(L"x=#5:40#; x-300")));
			Assert::AreEqual(114, get<int32_t>(ns.eval(L"myVar+mysens+myfunc1(mysens)")));
		}
		TEST_METHOD(Errors)
		{
			DumbSensor ts(L"temp", 24);
			DumbSensor ls(L"light", 2000);
			DumbRemote remote('t', 42);
			config_manager cm{ "config.db" };
			udns_resolver udns{ cm };
			motor_ctrl motc(L"blind", L"localhost", udns, cm);

			NScript ns;
			ns.set(L"myfunc", [](auto& p) {return p[0]; });
			ns.set(L"t", [&]() {return ts.value(); });
			ns.set(L"l", [&]() {return ls.value(); });
			Assert::AreEqual(value_t{ error_t::name_not_found }, ns.eval(L"my_func(3)"));
			Assert::AreEqual(value_t{ error_t::name_not_found }, ns.eval(L"my_var"));
			//rls.update();
			//Assert::AreEqual(value_t{ error_t::not_implemented }, rls.value());
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
			Assert::AreEqual(42l, cm.get(L"nkey", 0l));
			Assert::AreEqual(L"value"s, cm.get(L"skey", L""));
			cm.set(L"nkey", -42l);
			cm.set(L"skey", L"another");
			Assert::AreEqual(-42l, cm.get(L"nkey", 0l));
			Assert::AreEqual(L"another"s, cm.get(L"skey", L""));
			cm.save();
			config_manager cm1(p);
			cm1.load();
			Assert::AreEqual(-42l, cm.get(L"nkey", 0l));
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
			string test("abc\0\1\2ονυ", 9);
			ofs.write(test.c_str(), test.size());
			ofs.close();
			http_server srv(L"666", L"unit test server");
			wstring value;
			srv.add(L"/",	  [](auto&&, auto&&) { return std::make_tuple(content_type::html, L"MAIN"); });
			srv.add(L"/test", [](auto&&, auto&&) { return std::make_tuple(content_type::html, L"OK"); });
			srv.add(L"/some", [](auto&&, auto&&) { return std::make_tuple(content_type::html, L"YEP"); });
			srv.add(L"/file", p);
			srv.add_action(L"doit", [&value](auto&&, auto&& v) { value = v; });
			srv.start().get();

			wstring content;
			thread([&content]() { content = HttpClient().GetStringAsync({ L"http://localhost:666" }).get(); }).join();
			Assert::AreEqual(L"MAIN"s, content);
			thread([&content]() { content = HttpClient().GetStringAsync({ L"http://localhost:666/test" }).get(); }).join();
			Assert::AreEqual(L"OK"s, content);
			thread([&content]() { content = HttpClient().GetStringAsync({ L"http://localhost:666/some?doit=YES" }).get(); }).join();
			Assert::AreEqual(L"YEP"s, content);
			Assert::AreEqual(L"YES"s, value);

			IBuffer buf;
			array<uint8_t, 9> res;
			thread([&buf]() { buf = HttpClient().GetBufferAsync({ L"http://localhost:666/file" }).get(); }).join();
			DataReader::FromBuffer(buf).ReadBytes(res);
			Assert::AreEqual(0, memcmp(res.data(), test.data(), res.size()));
			filesystem::remove(p);
		}

	};
}