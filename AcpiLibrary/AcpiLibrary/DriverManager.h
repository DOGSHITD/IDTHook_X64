#pragma once
#include <Windows.h>
#include <tchar.h>
#include "Stdafx.h"

#define DRIVER_NAME L"AcpiFilter"
#define FILE_NAME_DRIVER_IMAGE TEXT("ntmdrv.sys")

using namespace System;

ref class DriverManager
{
public:
	DriverManager(void);
	~DriverManager();
	BOOL WINAPI LoadNTDriver();
	BOOL WINAPI UnloadNTDriver();
private:
	WCHAR* m_wsDriverName;
	WCHAR* m_wsDriverImgPath;
};
