#pragma once
#include <Windows.h>
#include <atlstr.h>
#include "DriverManager.h"

namespace AcpiLibrary
{
	public ref class DisplayIDTControl
	{
	public:
		DisplayIDTControl(void);
		~DisplayIDTControl();
		BOOL InvokeIDTLibrary(void);
	private:
		DriverManager ^driverManager;
	};
}