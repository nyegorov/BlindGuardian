// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include "winrt\base.h"
#include "winrt\Windows.ApplicationModel.Background.h"
#include "winrt\Windows.Data.Json.h"
#include "winrt\Windows.Devices.Gpio.h"
#include "winrt\Windows.Networking.Sockets.h"
#include "winrt\Windows.Storage.Streams.h"
#include "winrt\Windows.System.Threading.h"
#include "winrt\Windows.Web.Http.h"

#pragma comment(lib, "windowsapp")

// Headers for CppUnitTest
#include "CppUnitTest.h"

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <ctime>
#include <cvt\utf8>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <sstream>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>
#include <experimental\resumable>


using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;

