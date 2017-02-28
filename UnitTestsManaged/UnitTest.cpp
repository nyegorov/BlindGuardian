#include "stdafx.h"

#include "../RoomController/Value.h"
#include "../RoomController/Sensor.h"
#include "../RoomController/Rules.h"
#include "../RoomController/parser.h"

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace UnitTestsManaged
{
	[TestClass]
	public ref class UnitTest
	{
	private:
		TestContext^ testContextInstance;

	public: 
		/// <summary>
		///Gets or sets the test context which provides
		///information about and functionality for the current test run.
		///</summary>
		property Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ TestContext
		{
			Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ get()
			{
				return testContextInstance;
			}
			System::Void set(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ value)
			{
				testContextInstance = value;
			}
		};

		#pragma region Additional test attributes
		//
		//You can use the following additional attributes as you write your tests:
		//
		//Use ClassInitialize to run code before running the first test in the class
		//[ClassInitialize()]
		//static void MyClassInitialize(TestContext^ testContext) {};
		//
		//Use ClassCleanup to run code after all tests in a class have run
		//[ClassCleanup()]
		//static void MyClassCleanup() {};
		//
		//Use TestInitialize to run code before running each test
		//[TestInitialize()]
		//void MyTestInitialize() {};
		//
		//Use TestCleanup to run code after each test has run
		//[TestCleanup()]
		//void MyTestCleanup() {};
		//
		#pragma endregion 

		[TestMethod]
		void Parsing()
		{
			NScript ns;
			ns.set("myfunc", [](value_t p) {return p + p; });
			ns.set("myvar", value_t{ 42 });
			Assert::AreEqual(4ll, ns.eval("2*2").value);
			Assert::AreEqual(1ll, ns.eval("2*(5-3)==4").value);
			Assert::AreEqual(1ll, ns.eval("(1>2 || 1>=2 || 1<=2 || 1<2) && !(3==4) && (3!=4) ? 1 : 0").value);
			Assert::AreEqual(0ll, ns.eval("(2<=1 || 1<1 || 1>1 || 1<1) && !(3==3) && (3!=3) ? 1 : 0").value);
			Assert::AreEqual(42ll, ns.eval("x=42; x").value);
			Assert::AreEqual(1ll, ns.eval("x=1; y=2; x=x+y; y=y-x; x=x*y; x=x/y; x=x-1; x+y").value);
			Assert::AreEqual(48ll, ns.eval("x=3; MyFunc(x)+myVar").value);
			Assert::AreEqual(340ll, ns.eval("x=#5:40#; x").value);
		};
	};
}
