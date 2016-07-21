#include "stdafx.h"
#include "DriverManager.h"


#pragma comment(lib,"Advapi32.lib")
#include <vcclr.h>


using namespace System;
DriverManager::DriverManager(void)
{
	DebugPrint(("BOYCEHONG: DriverManager\n"));
	m_wsDriverName = new wchar_t[wcslen(DRIVER_NAME) + 1];
	wcscpy_s(m_wsDriverName, wcslen(DRIVER_NAME) + 1, DRIVER_NAME);
	pin_ptr<const wchar_t> wch = PtrToStringChars(AppDomain::CurrentDomain->BaseDirectory + gcnew String(FILE_NAME_DRIVER_IMAGE));
	size_t origsize = wcslen(wch) + 1;
	origsize = wcslen(wch) + 1;
	m_wsDriverImgPath = new wchar_t[origsize];
	wcscpy_s(m_wsDriverImgPath, origsize, wch);
}

DriverManager::~DriverManager()
{
	DebugPrint(("BOYCEHONG: ~ DriverManager\n"));
	if (m_wsDriverName != NULL)
	{
		delete m_wsDriverName;
	}
	if (m_wsDriverImgPath != NULL)
	{
		delete m_wsDriverImgPath;
	}
}

BOOL DriverManager::LoadNTDriver()
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;
	SC_HANDLE hServiceDDK = NULL;
	DebugPrint(("BOYCEHONG: LoadNTDriver\n"));
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		bRet = FALSE;
	}
	else
	{
		hServiceDDK = ::OpenService(hServiceMgr, m_wsDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK)
		{
			SERVICE_STATUS SvrSta;
			if (!ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta))
			{
				//CHECK(0,GetLastError());
			}
			if (!DeleteService(hServiceDDK))
			{
				//CHECK(0,GetLastError());
			}
			CloseServiceHandle(hServiceDDK);
		}
		hServiceDDK = CreateService(hServiceMgr,
			m_wsDriverName,
			m_wsDriverName,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			m_wsDriverImgPath,
			NULL, NULL, NULL, NULL, NULL);
		if (hServiceDDK == NULL)
		{
			//CHECK(0,GetLastError()); 
			bRet = FALSE;
		}
		else
		{
			bRet = StartService(hServiceDDK, NULL, NULL);
			if (!bRet)
			{
				//CHECK(0,GetLastError());
			}
			if (hServiceDDK)CloseServiceHandle(hServiceDDK);
		}
		if (hServiceMgr)CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}
BOOL WINAPI DriverManager::UnloadNTDriver()
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;
	SC_HANDLE hServiceDDK = NULL;
	SERVICE_STATUS SvrSta;
	
	DebugPrint(("BOYCEHONG: UnloadNTDriver\n"));
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//CHECK(0,GetLastError());
		bRet = FALSE;
	}
	else
	{
		hServiceDDK = OpenService(hServiceMgr, m_wsDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			//CHECK(0,GetLastError());
			bRet = FALSE;
		}
		else
		{
			ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta);
			if (!DeleteService(hServiceDDK))
			{
				//CHECK(0,GetLastError());
			}
			else bRet = TRUE;
			CloseServiceHandle(hServiceDDK);
		}
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}