/*
Code by BoyceHong.
Time at 2016.06.27.
Any problem cantact boyce_hong@asus.com
*/

#include "ntifs.h"
#include <windef.h>
#include <intrin.h>
#include "ntmdrv.h"
#include "asmLib.h"
#include <conio.h>
#include "EC.h"

#pragma warning(disable:4100)

#define MAX_PROCESSOR_NUMBER 8

/* Compile directives. */
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, MyDriver_UnSupportedFunction)
#endif

ULONG64 oldFunction = 0;

/*
typedef
VOID
(*PKINTERRUPT_ROUTINE) (
	VOID
	);

struct _KINTERRUPT;

// begin_ntddk begin_wdm begin_ntifs begin_ntosp

typedef
BOOLEAN
(*PKSERVICE_ROUTINE) (
	IN struct _KINTERRUPT *Interrupt,
	IN PVOID ServiceContext
	);

#define NORMAL_DISPATCH_LENGTH 106 // ntddk wdm
#define DISPATCH_LENGTH NORMAL_DISPATCH_LENGTH // ntddk wdm

// Interrupt object
typedef struct _KINTERRUPT {
	CSHORT Type;
	CSHORT Size;
	LIST_ENTRY InterruptListEntry;
	PKSERVICE_ROUTINE ServiceRoutine;
	PVOID ServiceContext;
	KSPIN_LOCK SpinLock;
	ULONG TickCount;
	PKSPIN_LOCK ActualLock;
	PKINTERRUPT_ROUTINE DispatchAddress;
	ULONG Vector;
	KIRQL Irql;
	KIRQL SynchronizeIrql;
	BOOLEAN FloatingSave;
	BOOLEAN Connected;
	CCHAR Number;
	BOOLEAN ShareVector;
	KINTERRUPT_MODE Mode;
	ULONG ServiceCount;
	ULONG DispatchCount;

#if defined(_AMD64_)
	PKTRAP_FRAME TrapFrame;
	PVOID Reserved;
	ULONG DispatchCode[DISPATCH_LENGTH];
#else
	ULONG DispatchCode[DISPATCH_LENGTH];
#endif

} KINTERRUPT;
*/


#if defined(_WIN64)
#if 0
extern "C" VOID show_idt(int i)
/*
i的取值可以是0.
*/
{
	AMD64_DESCRIPTOR idtr = { 0 };
	//	PVOID pIdtr = 0;
	SIZE_T r = 0;
	PVOID p = 0;
	int index = 0;
	int maximun = 0;

	PKIDTENTRY64 pkidte = 0;
	SIZE_T ISR = 0;
	//	USHORT us = 0;

	KeSetSystemAffinityThread((ULONG64)0x0001UL<<i);
	//pIdtr = (PVOID *)GetIdtr();
	//idtr = *((AMD64_DESCRIPTOR *)pIdtr);
	_disable();
	__sidt(&idtr);
	_enable();
	KeRevertToUserAffinityThread();

	DebugPrint(("BOYCEHONG: CPU%d Base:0x%X limit:%x\n",i, idtr.Base, idtr.Limit));
	DebugPrint(("BOYCEHONG: idtrContext:%x\n", *((PKIDTENTRY64)idtr.Base)));
/*
	p = &idtr.Pad[1];
	r = *(SIZE_T *)p;

	if (idtr.Pad[0] % sizeof(KIDTENTRY64) == 0) {//idtr.Pad[0] == 0xfff.
		maximun = idtr.Pad[0] / sizeof(KIDTENTRY64);
	}
	else {
		maximun = idtr.Pad[0] / sizeof(KIDTENTRY64);
		maximun++;//这个数也是256.
	}
*/
	pkidte = (PKIDTENTRY64)idtr.Base;
	for (; index < 256; index++)
	{
		PKIDTENTRY64 pkidte_t = &pkidte[index];

		ISR = 0;
		ISR = pkidte_t->IDT_ENTRY.OffsetHigh;
		ISR = (ISR << 32);
		ISR += (pkidte_t->IDT_ENTRY.OffsetLow + (pkidte_t->IDT_ENTRY.OffsetMiddle << 16));

		if (pkidte_t->IDT_ENTRY.IstIndex == 0) {
			DebugPrint(("BOYCEHONG: CPU%d Interrupt #0x%02x  Address:0x%p\n", i, index, ISR));
		}
		else {
			DebugPrint(("BOYCEHONG: CPU%d Interrupt #0x%02x  Address:0x%p\n", i, index, ISR));//还可以进一步获取Stack的信息。
		}
	}
}
#endif
 
/* number 8 means the supported max number of processors */
ULONG64 OriginalFunction[MAX_PROCESSOR_NUMBER];

/*
* Get the address of current work cpu's IDT table.
*/
AMD64_DESCRIPTOR GetIdtrAddress(int i) {
	AMD64_DESCRIPTOR idtr;

	/* get address of the IDT table */
	KeSetSystemAffinityThread((ULONG64)0x0001UL << i);
	_disable();
	__sidt(&idtr);
	_enable();
	KeRevertToUserAffinityThread();

	DebugPrint(("BOYCEHONG: CPU%d  Idtr: %x \n", i, idtr.Base));

	return idtr;
}


/*
* Get the address of the service descriptor.
*/
PKIDTENTRY64 GetTargetEntry(int i, UINT16 index) {
	/* allocate local variables */
	AMD64_DESCRIPTOR idtr;
	PKIDTENTRY64 pTargetEntry;

	idtr = GetIdtrAddress(i);

	/* get address of the interrupt entry we would like to hook */
	pTargetEntry = (KIDTENTRY64*)idtr.Base;
	pTargetEntry = &pTargetEntry[index];

	return pTargetEntry;
}

UINT8 log2(UINT32 value)
{
	if (1 == value)
		return 0;
	else
		return (1 + log2(value >> 1));
}

KEVENT g_ExitEvent,g_MtEvent;
KSPIN_LOCK NotifyLock = { 0 };
BOOL gExit = FALSE;

void display(void) {
	KeSetEvent(&g_MtEvent, 0, 0);
}

/*
* Hook the interrupt descriptor by overwriting its ISR pointer.
*/
VOID HookISR(UINT16 index, ULONG64 * hookFunction, BOOL IsRestore) {
	ULONG64 isr = 0;
	PKIDTENTRY64 pTargetEntry;
	int i = 0;
	unsigned __int64 CR0_Context = 0;

	for (; i < KeNumberProcessors; i++)
	{
		pTargetEntry = GetTargetEntry(i, index);
		isr = pTargetEntry->IDT_ENTRY.OffsetHigh;
		isr <<= 32;
		isr += (pTargetEntry->IDT_ENTRY.OffsetLow + (pTargetEntry->IDT_ENTRY.OffsetMiddle << 16));
		if (isr == hookFunction[i]) {
			DebugPrint(("BOYCEHONG: The service 0x%x for CPU%d already hooked.\n", index, i));
		}
		else {
			DebugPrint(("BOYCEHONG: Hooking interrupt %x: ISR 0x%p --> 0x%p.\r\n", index, isr, hookFunction[i]));
			if (!IsRestore) {
				OriginalFunction[i] = isr;
				DebugPrint(("BOYCEHONG: OriginalFunction[%d] 0x%p.\r\n", i, isr));
			}
			
			/* Set address of the IDT table */
			_disable();
			//DisableWriteProtect(&CR0_Context);
			pTargetEntry->IDT_ENTRY.OffsetHigh = (UINT32)(hookFunction[i] >> 32);
			pTargetEntry->IDT_ENTRY.OffsetMiddle = (UINT16)(hookFunction[i] >> 16);
			pTargetEntry->IDT_ENTRY.OffsetLow = (UINT16)hookFunction[i];
			//EnableWriteProtect(CR0_Context);
			_enable();
		}
		pTargetEntry = GetTargetEntry(i, index);
		isr = pTargetEntry->IDT_ENTRY.OffsetHigh;
		isr <<= 32;
		isr += (pTargetEntry->IDT_ENTRY.OffsetLow + (pTargetEntry->IDT_ENTRY.OffsetMiddle << 16));
		DebugPrint(("BOYCEHONG: Hook result ISR 0x%p.\r\n", isr));
	}
	oldFunction = OriginalFunction[0];
	SaveOriginalFunctionPtr((unsigned __int64)OriginalFunction);
	SaveHookFunction((unsigned __int64)display);
}

#else
extern "C" VOID show_idt(int i)
/*
i的取值可以是0.
*/
{
	//SIZE_T IDTR;
	X86_DESCRIPTOR idtr = { 0 };//A pointer to the memory location where the IDTR is stored.

	SIZE_T r = 0;
	PVOID p = 0;
	int index = 0;
	int maximun = 0;

	PKIDTENTRY pkidte;
	SIZE_T ISR = 0;

	KeSetSystemAffinityThread(i + 1);
	__sidt(&idtr);// http://msdn.microsoft.com/zh-cn/library/aa983358%28v=vs.120%29.aspx 另一个思路是自己实现：KeGetPcr()。
	KeRevertToUserAffinityThread();

	p = &idtr.Limit;
	r = *(SIZE_T *)p;

	pkidte = (PKIDTENTRY)r;

	/*
	其实直接：
	maximun = (idtr.Base + 1) / sizeof(KIDTENTRY);
	也可以。
	maximun一般等于256.
	*/
	if (idtr.Pad % sizeof(KIDTENTRY) == 0) {
		maximun = idtr.Pad / sizeof(KIDTENTRY);
	}
	else {
		maximun = idtr.Pad / sizeof(KIDTENTRY);
		maximun++;
	}

	for (; index < maximun; index++) //另一个思路是根据Limit来遍历，这个数一般是2047 == 0x7ff.
	{
		PKIDTENTRY pkidte_t = &pkidte[index];

		if (pkidte_t->ExtendedOffset) {
			ISR = pkidte_t->Offset + (pkidte_t->ExtendedOffset << 16);
			DebugPrint(("BOYCEHONG: CPU%d Interrupt #0x%02x  Address:0x%p\n", i, index, ISR));
		}
		else {//注意：pkidte_t->ExtendedOffset == 0的情况的分析。
			if (pkidte_t->Selector == 8) {
				DebugPrint(("BOYCEHONG: CPU%d  Unused Interrupt #0x%02x Offset:0x%x,Access:0x%x\n", i, index, pkidte_t->Offset, pkidte_t->Access));
			}
			else {
				DebugPrint(("BOYCEHONG: CPU%d  Unused Interrupt #0x%02x Offset:0x%x,Access:0x%x\n", i, index, pkidte_t->Selector, pkidte_t->Offset, pkidte_t->Access));
			}
		}
	}
}
#endif

/*
* Hook function.

void HookRoutine(void)
{
	ULONG currentActiveCPU = 0;

	KeLowerIrql(PASSIVE_LEVEL);
	SwitchToRing0();
	DebugPrint(("BOYCEHONG: HookRoutine Entry\n"));
	currentActiveCPU = KeGetCurrentProcessorNumber();
	DebugPrint(("BOYCEHONG: HookRoutine currentActiveCPU: %d\n", currentActiveCPU));
	if (currentActiveCPU < MAX_PROCESSOR_NUMBER) {
		DebugPrint(("BOYCEHONG: HookRoutine Mark1\n"));

		JmpAddress(OriginalFunction[currentActiveCPU]);
		//ReverseRingX(OriginalFunction[currentActiveCPU]);
	}
	else{
		DebugPrint(("BOYCEHONG: HookRoutine Mark3\n"));
	}
}
*/

/*
* MyDriver_Unload: called when the driver is unloaded.
*/
DRIVER_UNLOAD DriverUnload;
extern "C" VOID DriverUnload(__in PDRIVER_OBJECT  DriverObject) {
	/* local variables */
	PDEVICE_OBJECT pDev;
	UNICODE_STRING dosDeviceName;

	gExit = TRUE;
	//KeSetEvent(&g_MtEvent, 0, FALSE);
	KeWaitForSingleObject(&g_ExitEvent, Executive, KernelMode, 0, 0);
	KeResetEvent(&g_ExitEvent);
	/* restore the hook */
//	HookISR(0xb0, (ULONG64 *)OriginalFunction, TRUE);

	/* delete the driver */
	pDev = DriverObject->DeviceObject;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\SciDetectDriver");
	IoDeleteSymbolicLink(&dosDeviceName);
	IoDeleteDevice(pDev);
	DebugPrint(("BOYCEHONG: Driver %x Unloaded\n", pDev));
}

/*
* MyDriver_UnSupportedFunction: called when a major function is issued that isn't supported.
*/
NTSTATUS MyDriver_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;
	
	DebugPrint(("BOYCEHONG: MyDriver_UnSupportedFunction Called \r\n"));

	return NtStatus;
}

void __cdecl SystemMtThread(PVOID pvContext)
{
	unsigned long GPE_Status = 0;
	UINT8 i, eventNumber = 0;
	KIRQL irql;

	while (!gExit)
	{
		//KeWaitForSingleObject(&g_MtEvent, Executive, KernelMode, 0, 0);
		//KeResetEvent(&g_MtEvent);
		DebugPrint(("BOYCEHONG: g_MtEvent Signaled \r\n"));
		if (gExit)
			break;

		i = 0;
		
		while (!(GPE_Status = _inpd(GPE0_STATUS_BASE + 4 * i++)))
		if (4 == i)
		    break;

		if (!GPE_Status) {
		eventNumber = log2(GPE_Status) + (i - 1) * 32;
		DebugPrint(("BOYCEHONG: _L%02x\n", eventNumber));
		}
		else {
			//KeAcquireSpinLock(&NotifyLock, &irql);
			eventNumber = QueryCommand();
			//KeReleaseSpinLock(&NotifyLock, irql);
			if(eventNumber)
				DebugPrint(("BOYCEHONG: _Q%02x\n", eventNumber));
		}
		
	}

	gExit = FALSE;
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeSetEvent(&g_ExitEvent, 0, FALSE);
	DebugPrint(("BOYCEHONG: PsTerminateSystemThread \r\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
}

/*
* DriverEntry: entry point for drivers.
*/
extern "C" NTSTATUS DriverEntry(__in PDRIVER_OBJECT  pDriverObject, __in PUNICODE_STRING  pRegistryPath) 
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT32 index = 0;
	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING driverName, dosDeviceName;
	ULONG64 hookFunction[MAX_PROCESSOR_NUMBER] = { 0 };
	HANDLE hThread;
	//KIRQL oldIrql;

	DebugPrint(("BOYCEHONG: DriverEntry\n"));
	
	KeInitializeEvent(&g_MtEvent, NotificationEvent, 0);
	KeInitializeEvent(&g_ExitEvent, NotificationEvent, 0);
	KeInitializeSpinLock(&NotifyLock);
	if (!PsCreateSystemThread(&hThread, 0, 0, 0, 0, (PKSTART_ROUTINE)SystemMtThread, 0))
	{
		ZwClose(hThread);
	}

	DebugPrint(("BOYCEHONG: Driver run in CPU%d\n", KeGetCurrentProcessorNumber()));

	/*
	oldIrql = KeGetCurrentIrql();
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
	KeLowerIrql(oldIrql);
	*/

	RtlInitUnicodeString(&driverName, L"\\Device\\SciDetectDriver");
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\SciDetectDriver");

	__try {

		NtStatus = IoCreateDevice(pDriverObject, 0, &driverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);

		if (NtStatus == STATUS_SUCCESS) {
			/* MajorFunction: is a list of function pointers for entry points into the driver. */
			for (index = 0; index < IRP_MJ_MAXIMUM_FUNCTION; index++)
				pDriverObject->MajorFunction[index] = MyDriver_UnSupportedFunction;

			/* DriverUnload is required to be able to dynamically unload the driver. */
			pDriverObject->DriverUnload = DriverUnload;
			pDeviceObject->Flags |= 0;
			pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

			/* Create a Symbolic Link to the device. MyDriver -> \Device\MyDriver */
			IoCreateSymbolicLink(&dosDeviceName, &driverName);

			/* hook IDT */
/*			for (index = 0; index < MAX_PROCESSOR_NUMBER; index++) {
				hookFunction[index] = (ULONG64)HookRoutine;
			}
			HookISR(0xb0, (ULONG64 *)hookFunction, FALSE);
*/	
		}
		else {
			DebugPrint(("BOYCEHONG: IoCreateDevice failed\n"));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DebugPrint(("BOYCEHONG: GEXCEPTION in DriverEntry\n"));
		//KdPrint(("BOYCEHON: GEXCEPTION in DriverEntry\n"));
	}
	
	return NtStatus;
}
