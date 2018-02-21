// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <process.h>
#include <Windows.h>

#include <psapi.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include <tlhelp32.h>
#include <strsafe.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Ws2_32.lib")

#pragma comment(lib, "DbgHelp.Lib")
#pragma comment(lib, "ImageHlp")
#pragma comment(lib, "psapi")

// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
#include "APIHook.h"
#include "CrashDump.h"
#include "MemoryPool.h"
#include "LockfreeStack.h"
#include "LockfreeQueue.h"
#include "main.h"