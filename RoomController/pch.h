//
// pch.h
// Header for standard system include files.
//

#pragma once

#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#pragma comment(lib, "windowsapp")

#include "winrt\base.h"
#include "winrt\Windows.ApplicationModel.Background.h"
#include "winrt\Windows.Data.Json.h"
#include "winrt\Windows.Devices.Enumeration.h"
#include "winrt\Windows.Devices.Gpio.h"
#include "winrt\Windows.Devices.I2c.h"
#include "winrt\Windows.Foundation.Diagnostics.h"
#include "winrt\Windows.Networking.Sockets.h"
#include "winrt\Windows.Networking.Connectivity.h"
#include "winrt\Windows.Storage.Streams.h"
#include "winrt\Windows.System.Threading.h"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <codecvt>
#include <ctime>
#include <cvt\utf8>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <sstream>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include <pplawait.h>
#include <concurrent_queue.h>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;

namespace winrt::ABI::Windows::Foundation
{
template <> struct __declspec(uuid("3a14233f-a037-4ac0-a0ad-c4bb0bbf0111")) __declspec(novtable) AsyncOperationProgressHandler<bool, int> : impl_AsyncOperationProgressHandler<bool, int> {};
template <> struct __declspec(uuid("3a14233f-a037-4ac0-a0ad-c4bb0bbf0112")) __declspec(novtable) AsyncOperationWithProgressCompletedHandler<bool, int> : impl_AsyncOperationWithProgressCompletedHandler<bool, int> {};
template <> struct __declspec(uuid("3a14233f-a037-4ac0-a0ad-c4bb0bbf0113")) __declspec(novtable) IAsyncOperationWithProgress<bool, int> : impl_IAsyncOperationWithProgress<bool, int> {};
}

#undef min
#undef max