#pragma once

#pragma comment(lib, "windowsapp") 

#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include "winrt\base.h"
#include "winrt\Windows.Devices.Enumeration.h"
#include "winrt\Windows.Networking.Sockets.h"
#include "winrt\Windows.Networking.Connectivity.h"
#include "winrt\Windows.Storage.Streams.h"
#include "winrt\Windows.Data.Json.h"

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

#include <concurrent_queue.h>
