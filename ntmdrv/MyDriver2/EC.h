#pragma once
#include <conio.h>
#include <NTDDK.h>
#include <malloc.h>

//typedef unsigned char UINT8;
//typedef unsigned short  UINT16;
//typedef unsigned long UINT32;

#define EC_SC     0x66
#define EC_DATA   0x62

typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef unsigned __int64    UINT64, *PUINT64;

#if DBG

#define TRAP()                      DbgBreakPoint()

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()

#define DebugPrint(_x_)

#endif

void WriteCommand(UINT8 SMBAddr, UINT8 SMBData);

UINT8 ReadCommand(UINT8 SMBAddr);

UINT8 PrintBatterySN(void);

UINT8 QueryCommand(void);