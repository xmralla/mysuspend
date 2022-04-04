#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <string>
#include <thread>
#include <map>
#include <tuple>
#include <vector>
#include <iomanip>

#include <windows.h>
#include <psapi.h>
#include <time.h>
#include <dbghelp.h>
#include <string.h>
#include <intrin.h>

const auto PSLIST_SIZE = 1024;

#include "PsInfo.h"
#include "DrvInfo.h"

typedef std::tuple<std::string, std::string> details;
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<float> fsec;
typedef std::map<DWORD, PsInfo*> Pid2PsInfoMap;
typedef std::map<LPVOID, DrvInfo*> Addr2DriverInfoMap;
