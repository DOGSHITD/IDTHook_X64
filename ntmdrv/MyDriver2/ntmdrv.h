#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <initguid.h>
#include <NTDDK.h>
#include <wdmguid.h>
//#include <GetIdtr.h>
#ifdef __cplusplus
}
#endif

#if DBG

#define TRAP()                      DbgBreakPoint()

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()

#define DebugPrint(_x_)

#endif

#define DEVICE_NAME L"\\Device\\SciDetectDriver"
#define SYMBOLINK_NAME L"\\DosDevices\\SciDetectDriver"

#define GPE0_STATUS_BASE 0x1880

#if defined(_WIN64)

#pragma pack(push)
#pragma pack(1)
// Special Registers for AMD64.
typedef struct _AMD64_DESCRIPTOR {
	//USHORT Pad[3];
	USHORT Limit;
	ULONG64 Base;
} AMD64_DESCRIPTOR, *PAMD64_DESCRIPTOR;

// Define Interrupt Descriptor Table (IDT) entry structure and constants.
typedef union _KIDTENTRY64 {
	struct {
		USHORT OffsetLow;
		USHORT Selector;
		USHORT IstIndex : 3;
		USHORT Reserved0 : 5;
		USHORT Type : 5;
		USHORT Dpl : 2;
		USHORT Present : 1;
		USHORT OffsetMiddle;
		ULONG OffsetHigh;
		ULONG Reserved1;
	}IDT_ENTRY;

	ULONG64 Alignment;
} KIDTENTRY64, *PKIDTENTRY64;

typedef union _KIDT_HANDLER_ADDRESS {
	struct {
		USHORT OffsetLow;
		USHORT OffsetMiddle;
		ULONG OffsetHigh;
	}ADDRESS_STRUCT;

	ULONG64 Address;
} KIDT_HANDLER_ADDRESS, *PKIDT_HANDLER_ADDRESS;
#pragma pack(pop)

/*
#define KiGetIdtFromVector(Vector)	&KeGetPcr()->IdtBase[HalVectorToIDTEntry(Vector)]

#define KeGetIdtHandlerAddress(Vector,Addr) {	\
KIDT_HANDLER_ADDRESS Handler;	\
PKIDTENTRY64 Idt;	\
Idt = KiGetIdtFromVector(Vector);	\
Handler.ADDRESS_STRUCT.OffsetLow = Idt->IDT_ENTRY.OffsetLow;		\
Handler.ADDRESS_STRUCT.OffsetMiddle = Idt->IDT_ENTRY.OffsetMiddle;	\
Handler.ADDRESS_STRUCT.OffsetHigh = Idt->IDT_ENTRY.OffsetHigh;	\
*(Addr) = (PVOID)(Handler.Address);	\
}

#define KeSetIdtHandlerAddress(Vector,Addr) {	\
KIDT_HANDLER_ADDRESS Handler;	\
PKIDTENTRY64 Idt;	\
Idt = KiGetIdtFromVector(Vector);	\
Handler.Address = (ULONG64)(Addr);	\
Idt->IDT_ENTRY.OffsetLow = Handler.ADDRESS_STRUCT.OffsetLow;		\
Idt->IDT_ENTRY.OffsetMiddle = Handler.ADDRESS_STRUCT.OffsetMiddle;	\
Idt->IDT_ENTRY.OffsetHigh = Handler.ADDRESS_STRUCT.OffsetHigh;	\
}
*/
#else

#pragma pack(push)
#pragma pack(1)
// Special Registers for i386
typedef struct _X86_DESCRIPTOR {
	USHORT Pad;
	USHORT Limit;
	ULONG Base;
} X86_DESCRIPTOR, *PX86_DESCRIPTOR;


// Entry of Interrupt Descriptor Table (IDTENTRY)
typedef struct _KIDTENTRY {
	USHORT Offset;
	USHORT Selector;
	USHORT Access;
	USHORT ExtendedOffset;
} KIDTENTRY;
typedef KIDTENTRY *PKIDTENTRY;
#pragma pack(pop)

// begin_nthal
//
// Macro to set address of a trap/interrupt handler to IDT
//
#define KiSetHandlerAddressToIDT(Vector, HandlerAddress) {
UCHAR IDTEntry = HalVectorToIDTEntry(Vector);
ULONG Ha = (ULONG)HandlerAddress;
KeGetPcr()->IDT[IDTEntry].ExtendedOffset = HIGHWORD(Ha);
KeGetPcr()->IDT[IDTEntry].Offset = LOWWORD(Ha);
}

//
// Macro to return address of a trap/interrupt handler in IDT
//
#define KiReturnHandlerAddressFromIDT(Vector)
MAKEULONG(KiPcr()->IDT[HalVectorToIDTEntry(Vector)].ExtendedOffset, KiPcr()->IDT[HalVectorToIDTEntry(Vector)].Offset)

#endif

//extern "C" VOID show_idt(int i);
AMD64_DESCRIPTOR GetIdtrAddress(int i);
PKIDTENTRY64 GetTargetEntry(int i, UINT16 index);
VOID HookISR(UINT16 index, ULONG64 * hookFunction, BOOL IsRestore);

UINT8 log2(UINT32 value);
extern "C" void display(void);

extern "C" VOID DriverUnload(__in PDRIVER_OBJECT DriverObject);
extern "C" NTSTATUS DriverEntry(__in PDRIVER_OBJECT  pDriverObject, __in PUNICODE_STRING  pRegistryPath);
extern "C" NTSTATUS MyDriver_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);