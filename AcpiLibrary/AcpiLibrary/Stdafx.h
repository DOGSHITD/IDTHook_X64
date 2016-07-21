// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include <Windows.h>
#include <tchar.h>

#define SERVICE_NAME "DisplayIDT"
#define DEVCIE_NAME TEXT("\\\\.\\DisplayIDTDevice")

#if DBG

#define TRAP()                      DbgBreakPoint()
#define DebugPrint(_x_) DbgPrint _x_

#else

#define TRAP()

#define DebugPrint(_x_)

#endif