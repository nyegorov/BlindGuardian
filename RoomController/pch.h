//
// pch.h
// Header for standard system include files.
//

#pragma once

#pragma comment(lib, "windowsapp")

#include "winrt\base.h"
#include "winrt\Windows.ApplicationModel.Background.h"
#include "winrt\Windows.Data.Json.h"
#include "winrt\Windows.Devices.Gpio.h"
#include "winrt\Windows.Networking.Sockets.h"
#include "winrt\Windows.Storage.Streams.h"
#include "winrt\Windows.System.Threading.h"

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

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
