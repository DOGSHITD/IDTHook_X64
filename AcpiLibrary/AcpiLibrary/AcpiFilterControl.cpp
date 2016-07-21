#include "stdafx.h"
#include "AcpiFilterControl.h"
#include <SetupAPI.h>
#pragma comment(lib,"setupapi.lib")

#include <vcclr.h>
#include <atlstr.h>
#include<iostream>

using namespace System;
using namespace AcpiLibrary;
using namespace System::Threading;
using namespace System::Collections::Generic;

DisplayIDTControl::DisplayIDTControl(void)
{
	DebugPrint(("BOYCEHONG: DisplayIDTControl\n"));
	driverManager = gcnew DriverManager();
	driverManager->LoadNTDriver();
}
DisplayIDTControl::~DisplayIDTControl()
{
	DebugPrint(("BOYCEHONG: ~ DisplayIDTControl\n"));
	driverManager->UnloadNTDriver();
	driverManager = nullptr;
}

BOOL DisplayIDTControl::InvokeIDTLibrary(void)
{
	BOOL status = TRUE;

	std::cout << "BOYCEHONG: Invoke IDTlibrary\n";

	DebugPrint(("BOYCEHONG: Invoke IDTlibrary\n"));

	return status;
}