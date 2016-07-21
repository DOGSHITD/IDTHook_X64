#include "ntmdrv.h"

typedef NTSTATUS (FASTCALL *pIofCallDriver)( IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
extern ULONG g_ulACPIHandleOffset;
extern ULONG g_ulACPICallBackOffset;
extern ULONG g_ulOSVersion;
extern PKEVENT g_pApMethodEvent;
extern PDEVICE_OBJECT acpihal_pdo;
extern PDEVICE_OBJECT ec_pdo;
extern KEVENT g_PendingEvent;
extern KSPIN_LOCK g_LiskSpinLock;
extern KEVENT g_MtEvent;
//////////////////////////////////////////////
UCHAR g_ucDriverQuit=0;
ULONG g_llRefCount=0;
//UCHAR g_bRunService=0;
UCHAR ucHooked=0;
pIofCallDriver Old_pIofCallDriver=0;
pIofCallDriver Old_pIofCompleteRequest=0;
ULONG oData=0;
extern PDEVICE_EXTENSION g_pdx;

PDRIVER_DISPATCH oldIoControlDispachRoutine;
PIO_COMPLETION_ROUTINE oldCallMethodCompletionRoutine;
typedef struct OldIrpStackInformation
{
	PIO_STACK_LOCATION oldCurrentStackLocation;
	CHAR oldCurrentLocation;
	CHAR oldStackCount;
	PVOID oldContext;
	UCHAR oldControl;
}*POldIrpStackInformation;

DRIVER_DISPATCH HookIoControlDispatchRoutine;
IO_COMPLETION_ROUTINE HookCallMethodCompletionRoutine;
/////////////////////////////////////////////


VOID AcpiHalHook()
{	
	KdPrint(("Acpi: AcpiHalHook\n"));
	KIRQL oldIrql;
	oldIrql = KeRaiseIrqlToDpcLevel();
	oldIoControlDispachRoutine = acpihal_pdo->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
	acpihal_pdo->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HookIoControlDispatchRoutine;
	ucHooked=1;
	KeLowerIrql(oldIrql);
}
VOID AcpiHalUnHook()
{
	KdPrint(("UnHookpIofCallDriver\n"));
	if(ucHooked)
	{  
		KIRQL oldIrql;
		oldIrql = KeRaiseIrqlToDpcLevel();
		acpihal_pdo->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = oldIoControlDispachRoutine;

		KeLowerIrql(oldIrql);
	}
	return;
}

NTSTATUS HookIoControlDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{		
	static long index;
	DebugPrint(("ACPI: HookIoControlDispatchRoutine pIrp:DebugPrint %08lx\n", pIrp));
	//DebugPrint(("ACPI: Cancel %d Thread %08lx\n",pIrp->Cancel,pIrp->Tail.Overlay.Thread));
	//DebugPrint(("ACPI: Thread offset %02lx\n",FIELD_OFFSET(IRP, Tail.Overlay.Thread)));
	//if(pIrp->Cancel != 0 || pIrp->Tail.Overlay.Thread==0)
	//{
	//	NTSTATUS status = oldIoControlDispachRoutine(pDevObj,pIrp);

	//	return status;
	//}
	PIO_STACK_LOCATION pIrpStack;
	PIO_STACK_LOCATION pIoStackBuffer;
	POldIrpStackInformation pOldIrpStackInformation;
	pIoStackBuffer = (PIO_STACK_LOCATION)ExAllocatePoolWithTag(POOL_TYPE::NonPagedPool,10*sizeof(IO_STACK_LOCATION),'BatS'^index);
	pOldIrpStackInformation = (POldIrpStackInformation)ExAllocatePoolWithTag(POOL_TYPE::NonPagedPool,10*sizeof(IO_STACK_LOCATION),'ISIO'^index);
	index ++;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG dwIoControlCode ;
	//DebugPrint(("ACPI: IrpStack -> MajorFunction %02x\n", pIrpStack -> MajorFunction));
	if(pIrpStack -> MajorFunction != IRP_MJ_DEVICE_CONTROL)
	{
		DebugPrint(("ACPI: [BOYCEHONG] IRP_MJ_DEVICE_CONTROL\n"));
		return STATUS_UNSUCCESSFUL;
	}
	dwIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	DebugPrint(("ACPI: dwIoControlCode %08lx\n", dwIoControlCode));
	switch (dwIoControlCode)
	{
	case IOCTL_ACPI_ASYNC_EVAL_METHOD:
		DebugPrint(("ACPI: IOCTL_ACPI_ASYNC_EVAL_METHOD\n"));
		break;
	case IOCTL_ACPI_EVAL_METHOD:
		DebugPrint(("ACPI: IOCTL_ACPI_EVAL_METHOD\n"));
		break;
	case IOCTL_ACPI_EVAL_METHOD_EX:
		DebugPrint(("ACPI: IOCTL_ACPI_EVAL_METHOD_EX\n"));
		break;
	case IOCTL_ACPI_ASYNC_EVAL_METHOD_EX:
		DebugPrint(("ACPI: IOCTL_ACPI_ASYNC_EVAL_METHOD_EX\n"));
		break;
	default:
		break;
	}
	PIO_STACK_LOCATION stack =IoGetCurrentIrpStackLocation(pIrp);
	if( g_pApMethodEvent)
	{
	DebugPrint(("ACPI: g_pApMethodEvent %08lx\n", g_pApMethodEvent));
		if(dwIoControlCode==IOCTL_ACPI_EVAL_METHOD  ||  dwIoControlCode == IOCTL_ACPI_ASYNC_EVAL_METHOD 
			|| dwIoControlCode == IOCTL_ACPI_EVAL_METHOD_EX ||  dwIoControlCode == IOCTL_ACPI_ASYNC_EVAL_METHOD_EX)
		{
	//DebugPrint(("ACPI: IOCTL_ACPI_EVAL_METHOD %08lx (0032c004)\n", IOCTL_ACPI_EVAL_METHOD));
	//DebugPrint(("ACPI: CurrentLocation %d\n",pIrp->CurrentLocation));
			DealTrace(pDevObj,pIrp,1);
			oldCallMethodCompletionRoutine = pIrpStack->CompletionRoutine;
			if(oldCallMethodCompletionRoutine == NULL)
			{
				//DebugPrint(("ACPI: oldCallMethodCompletionRoutine is NULL\n"));
			}

			pOldIrpStackInformation->oldCurrentStackLocation = pIrpStack;
			pOldIrpStackInformation->oldCurrentLocation = pIrp->CurrentLocation;
			//DebugPrint(("ACPI: StackCount %d\n",pIrp->StackCount));
			pOldIrpStackInformation->oldStackCount = pIrp->StackCount;
			pOldIrpStackInformation->oldContext = pIrpStack->Context;
			pOldIrpStackInformation->oldControl = pIrpStack->Control;

			RtlCopyMemory( pIoStackBuffer + (IO_STACK_BUFFER_MAX_SIZE -1), pIrpStack, sizeof(IO_STACK_LOCATION));
			pIrp->Tail.Overlay.CurrentStackLocation = pIoStackBuffer + (IO_STACK_BUFFER_MAX_SIZE -1);
			pIrp->CurrentLocation = IO_STACK_BUFFER_MAX_SIZE - 1;
			pIrp->StackCount = IO_STACK_BUFFER_MAX_SIZE;

			IoCopyCurrentIrpStackLocationToNext(pIrp);
			IoSetCompletionRoutine(pIrp,HookCallMethodCompletionRoutine,pOldIrpStackInformation,TRUE,TRUE,TRUE);
			pIrp->CurrentLocation--;
			pIrp->Tail.Overlay.CurrentStackLocation--;
		}
		//DebugPrint(("ACPI: pDevObj %08lx, pIrp %08lx\n",pDevObj, pIrp));
		//DebugPrint(("ACPI: currentStackLocation %08lx\n",pIrp->Tail.Overlay.CurrentStackLocation));
		//DebugPrint(("ACPI: IoControlCode %08lx\n",pIrp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.IoControlCode));
		//DealTrace(pDevObj,pIrp,0);
	}
	return oldIoControlDispachRoutine(pDevObj,pIrp);
	//return STATUS_SUCCESS;
}
NTSTATUS
HookCallMethodCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP pIrp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{	
	DebugPrint(("ACPI: [BOYCEHONG] HookCallMethodCompletionRoutine\n"));
	PIO_STACK_LOCATION pIrpStack =IoGetCurrentIrpStackLocation(pIrp);
	PIO_STACK_LOCATION pIoStackBuffer;
	POldIrpStackInformation pOldIrpStackInformation = (POldIrpStackInformation)Context;
	pIoStackBuffer = pIrpStack - pIrp->CurrentLocation;
	PIO_STACK_LOCATION pNextIrpStack =IoGetNextIrpStackLocation(pIrp);
	if(pIrp->Tail.Apc.Thread == 0)
	{
		//TRAP();
	}
	if(pIrpStack != NULL)
	{
	//DebugPrint(("ACPI: DeviceObject %08lx, g_pApMethodEvent %08lx\n",DeviceObject , g_pApMethodEvent));
		if(/*DeviceObject  &&*/  g_pApMethodEvent)
		{
			ULONG dwIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
			//DebugPrint(("ACPI: dwIoControlCode %08lx\n",dwIoControlCode));
			//if(dwIoControlCode==IOCTL_ACPI_EVAL_METHOD  ||  IOCTL_ACPI_EVAL_METHOD==dwIoControlCode+4)
			{
				DealTrace(DeviceObject,pIrp,0);
			}
			//RtlCopyMemory(oldCurrentStackLocation - oldCurrentLocation,pIoStackBuffer + (IO_STACK_BUFFER_MAX_SIZE -1 - oldCurrentLocation), oldCurrentLocation * IO_STACK_LOCATION);
			RtlCopyMemory( pOldIrpStackInformation->oldCurrentStackLocation, pIoStackBuffer + (IO_STACK_BUFFER_MAX_SIZE -2), FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
			pIrp->CurrentLocation = pOldIrpStackInformation->oldCurrentLocation;
			pIrp->Tail.Overlay.CurrentStackLocation = pOldIrpStackInformation->oldCurrentStackLocation;
			pIrp->StackCount = pOldIrpStackInformation->oldStackCount;
		}
	}
	if(oldCallMethodCompletionRoutine != NULL )
	{
		if(pOldIrpStackInformation->oldControl & pNextIrpStack->Control)
		{
			oldCallMethodCompletionRoutine(DeviceObject,pIrp,pOldIrpStackInformation->oldContext);
		}
	}
	//pIrpStack->CompletionRoutine = oldCallMethodCompletionRoutine;
	ExFreePool(pIoStackBuffer);
	return STATUS_SUCCESS;
}
//CHAR *irpname[] = 
//{
//"IRP_MJ_CREATE",
//"IRP_MJ_CREATE_NAMED_PIPE",
//"IRP_MJ_CLOSE",
//"IRP_MJ_READ",
//"IRP_MJ_WRITE",
//"IRP_MJ_QUERY_INFORMATION",
//"IRP_MJ_SET_INFORMATION",
//"IRP_MJ_QUERY_EA",
//"IRP_MJ_SET_EA",
//"IRP_MJ_FLUSH_BUFFERS",
//"IRP_MJ_QUERY_VOLUME_INFORMATION",
//"IRP_MJ_SET_VOLUME_INFORMATION",
//"IRP_MJ_DIRECTORY_CONTROL",
//"IRP_MJ_FILE_SYSTEM_CONTROL",
//"IRP_MJ_DEVICE_CONTROL",
//"IRP_MJ_INTERNAL_DEVICE_CONTROL",
//"IRP_MJ_SHUTDOWN",
//"IRP_MJ_LOCK_CONTROL",
//"IRP_MJ_CLEANUP",
//"IRP_MJ_CREATE_MAILSLOT",
//"IRP_MJ_QUERY_SECURITY",
//"IRP_MJ_SET_SECURITY",
//"IRP_MJ_POWER",
//"IRP_MJ_SYSTEM_CONTROL",
//"IRP_MJ_DEVICE_CHANGE",
//"IRP_MJ_QUERY_QUOTA",
//"IRP_MJ_SET_QUOTA",
//"IRP_MJ_PNP",
//};