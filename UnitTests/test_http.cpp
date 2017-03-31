#include "pch.h"
#include "CppUnitTest.h"

#include "../RoomController/rules_db.h"
#include "../RoomController/http_server.h"

using namespace std;
using namespace std::experimental;
using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Storage::Streams;

namespace UnitTests
{		
	TEST_MODULE_INITIALIZE(InitTest)
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	TEST_CLASS(UnitTest)
	{
	public:
		
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