#include "ntmdrv.h"
extern ULONG g_ulACPIHandleOffset;
extern ULONG g_ulACPICallBackOffset;
extern ULONG g_ulIndex;
extern PDEVICE_OBJECT acpihal_pdo;
extern PDEVICE_EXTENSION g_pdx;
extern KERNEL_DEVICE_INFO *pRegHeader;
extern KSPIN_LOCK RegListLock;
extern PKEVENT g_pApNotifyEvent;
extern KSPIN_LOCK NotifyLock;
extern ULONG g_ulNotifyCount;

extern PIRP pCurrentIrp;

extern NOTIFY_DATA NotifyBuffer[256];
VOID ASUSACPIDeviceNotifyHandler(PVOID Context,ULONG NotifyValue)
{
	KdPrint(("Context=0x%X,NotifyValue=0x%X\n",Context,NotifyValue));
	KERNEL_DEVICE_INFO *pTemp,*pInfo=(KERNEL_DEVICE_INFO*)g_pdx->pDeviceNofityInfo;
	UCHAR ucValid=0;
	while(pInfo)
	{
		pTemp=pInfo;
		pInfo=(KERNEL_DEVICE_INFO*)pInfo->pList.Blink;
		if(pTemp->pdo==Context)
		{
			ucValid=1;
			if(pTemp->ulHooked)
			{
				if(pTemp->pCallBack)
				{
					KdPrint(("Invoking Old RoutineX\n"));
					pTemp->pCallBack(pTemp->pParam,NotifyValue);
				}
			}break;
		}
		if(pInfo==g_pdx->pDeviceNofityInfo)break;
	}
	if(ucValid)
	{
		KIRQL irql;
		KeAcquireSpinLock(&NotifyLock,&irql);
		if(g_ulNotifyCount>255)
			g_ulNotifyCount=0;
		NotifyBuffer[g_ulNotifyCount].pdo=(PDEVICE_OBJECT)Context;
		NotifyBuffer[g_ulNotifyCount].ulNotifyValue=NotifyValue;
		g_ulNotifyCount++;
		KeReleaseSpinLock(&NotifyLock,irql);
		if(g_pApNotifyEvent)
			KeSetEvent(g_pApNotifyEvent,0,0);
	}
}
NTSTATUS OnAcpiRequestComplete(IN PDEVICE_OBJECT fdo,IN PIRP Irp,IN PKEVENT pev)
{
    KeSetEvent(pev, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}
NTSTATUS GetAcpiInterfaces (  PDEVICE_OBJECT Pdo, PACPI_INTERFACE_STANDARD PAcpiStruct)
{
    KEVENT              event;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    PAGED_CODE();
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );
	irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,Pdo,NULL,0,NULL,&event,&ioStatus);
    if (!irp) {
        status= STATUS_INSUFFICIENT_RESOURCES;
    }else
	{
		irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		irp->IoStatus.Information = 0;
		irpSp = IoGetNextIrpStackLocation( irp );
		irpSp->MajorFunction = IRP_MJ_PNP;
		irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
		irpSp->Parameters.QueryInterface.InterfaceType          = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
		irpSp->Parameters.QueryInterface.Version                = 1;
		irpSp->Parameters.QueryInterface.Size                   = sizeof (ACPI_INTERFACE_STANDARD);
		irpSp->Parameters.QueryInterface.Interface              = (PINTERFACE)PAcpiStruct;
		irpSp->Parameters.QueryInterface.InterfaceSpecificData  = NULL;
		status = IoCallDriver( Pdo, irp );
		if (status == STATUS_PENDING)
		{
			KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
			status = ioStatus.Status;
		}
	}
    return status;
}
PVOID ReplaceACPIHandle(PVOID in_pdo,PVOID ParentScope)
{
	//KdPrint(("ReplaceACPIHandle Input:0x%X,0x%X\n",in_pdo,ParentScope));
	PVOID pvRet=0;
	PVOID temp;
	RTL_OSVERSIONINFOW osvx={0};
	osvx.dwOSVersionInfoSize=sizeof(RTL_OSVERSIONINFOW);
	RtlGetVersion(&osvx);
	temp = ((PDEVICE_OBJECT)in_pdo)->DeviceObjectExtension->DeviceObject->DeviceExtension;
	temp = (PVOID)((ULONG_PTR)temp + g_ulACPIHandleOffset);
	if((osvx.dwMajorVersion == 6 && osvx.dwMinorVersion == 3) || osvx.dwMajorVersion == 10)
	{
		temp = *((PVOID*)temp);
	}
	pvRet = *(PVOID*)temp;
	*(PVOID*)temp = ParentScope;
	return pvRet;
	/*__asm
	{
		push eax
		push ebx
		push ecx
		push edx
		mov ecx,in_pdo
		mov ecx,[ecx+0xb0]
		mov ecx,[ecx+4]
		mov ecx,[ecx+0x28]
		mov edx,g_ulACPIHandleOffset
		add ecx,edx
		mov ebx,[ecx]
		mov pvRet,ebx
		mov ebx,ParentScope
		mov [ecx],ebx
		pop edx
		pop ecx
		pop ebx
		pop eax
	}*/
	//KdPrint(("ReplaceACPIHandle Returns:0x%X\n",pvRet));
	return pvRet;
}
PVOID GetAcpiHandle(PVOID in_pdo)
{
	//KdPrint(("GetAcpiHandle Input:0x%X\n",in_pdo));
	PVOID pvRet=0;
	PVOID temp;
	RTL_OSVERSIONINFOW osvx={0};
	osvx.dwOSVersionInfoSize=sizeof(RTL_OSVERSIONINFOW);
	RtlGetVersion(&osvx);
	temp = ((PDEVICE_OBJECT)in_pdo)->DeviceObjectExtension->DeviceObject->DeviceExtension;
	pvRet = *((PVOID *)((ULONG_PTR)temp + g_ulACPIHandleOffset));
	if((osvx.dwMajorVersion == 6 && osvx.dwMinorVersion == 3) || osvx.dwMajorVersion == 10)
	{
		pvRet = *((PVOID*)pvRet);
	}
	return pvRet;
	/*__asm
	{
		push eax
		push ebx
		push ecx
		push edx
		mov ecx,temp
		mov edx,g_ulACPIHandleOffset
		add ecx,edx
		mov ebx,[ecx]
		mov pvRet,ebx
		pop edx
		pop ecx
		pop ebx
		pop eax
	}*/
	/*__asm
	{
		push eax
		push ebx
		push ecx
		push edx
		mov ecx,in_pdo
		mov ecx,[ecx+0xb0]
		mov ecx,[ecx+4]
		mov ecx,[ecx+0x28]
		mov edx,g_ulACPIHandleOffset
		add ecx,edx
		mov ebx,[ecx]
		mov pvRet,ebx
		pop edx
		pop ecx
		pop ebx
		pop eax
	}*/
	//KdPrint(("GetAcpiHandle Returns:0x%X\n",pvRet));
	return pvRet;
}
ACPI_OBJECT *GetAcpiObjectRoot(ACPI_OBJECT* in_pObject)
{
	//KdPrint(("ACPI: GetAcpiObjectRoot Input:0x%X\n",in_pObject));
	ACPI_OBJECT* pResult=in_pObject;
	while(pResult&&pResult->pParent)
	{
		pResult=pResult->pParent;
	}
	//KdPrint(("GetAcpiObjectRoot Returns:0x%X\n",pResult));
	return pResult;
}
ULONG GetAcpiObjectCount(ACPI_OBJECT* in_pObject,ACPI_OBJECT_AP* in_pvBuffer,ULONG in_ulSize)
{
	////KdPrint(("GetAcpiObjectCount Input:0x%X\n",in_pObject));
	ULONG ulRet=0;
	ACPI_OBJECT *pTemp=in_pObject;
	ACPI_OBJECT *pTemp1=in_pObject;
	do{
		if(pTemp)
		{
			ulRet++;
			if(in_ulSize  &&  in_pvBuffer)
			{
				if(sizeof(ACPI_OBJECT_AP)*(g_ulIndex+1)<in_ulSize)
				{
					memcpy(&in_pvBuffer[g_ulIndex],(PVOID)(pTemp),sizeof(ACPI_OBJECT));
					in_pvBuffer[g_ulIndex].Tag=pTemp;
					g_ulIndex++;
				}else
				{
					//KdPrint(("Buffer Over Flow index=%ld\n",g_ulIndex));
				}
			}
			ulRet+=GetAcpiObjectCount((ACPI_OBJECT*)((ULONG_PTR)(pTemp->pChild)),in_pvBuffer,in_ulSize);		
		}else
		{
			break;
		}
		pTemp=pTemp->pNext;
	}while(pTemp  &&  pTemp!=pTemp1);
	////KdPrint(("GetAcpiObjectCount Returns:0x%X\n",ulRet));
	return ulRet;
}

ULONG GetDriverDeviceCount(PDRIVER_OBJECT pDrv)
{
	ULONG ulRet=0,ulReturnValue=0;
	WCHAR wszTemp[260]={0};
	if(pDrv)
	{
		PDEVICE_OBJECT pdo=pDrv->DeviceObject;
		while(pdo)
		{
			ulRet++;
			pdo=pdo->NextDevice;
		}
	}
	return ulRet;
}
NTSTATUS EvaluateACPIMethod( PDEVICE_OBJECT   Pdo,ULONG MethodName,ULONG ArgumentCount,PVOID in_Buffer,ULONG in_iBufferSize,PVOID out_Buffer,ULONG in_oBufferSize,ULONG_PTR *out_ulRets)
{
	   // ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX  inputBuffer;
    //ACPI_EVAL_OUTPUT_BUFFER                outputBuffer;
    //NTSTATUS                               status;
    //PACPI_METHOD_ARGUMENT                  argument;

    //// Omitted: bounds checking on Argument1 value.



    //// Fill in the input data
    //memcpy_s(inputBuffer.MethodName,255,&MethodName,4);
    ////inputBuffer.IntegerArgument  =  Argument1;
    //inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;

    //// Send the request along
    //status = SendDownStreamIrp(
    //   Pdo,
    //   IOCTL_ACPI_EVAL_METHOD_EX,
    //   &inputBuffer,
    //   sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX),
    //   &outputBuffer,
    //   sizeof(ACPI_EVAL_OUTPUT_BUFFER)
    //   );

    //return status;

	 NTSTATUS                               status=STATUS_UNSUCCESSFUL;
	 ACPI_EVAL_INPUT_BUFFER_COMPLEX         *pMethodBuffer=0;
	 ULONG ulMethodBufferSize=sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX)-sizeof(ACPI_METHOD_ARGUMENT)+max(in_iBufferSize,sizeof(ACPI_METHOD_ARGUMENT));
	 *out_ulRets=0;
	 if(  !(Pdo   &&  in_Buffer  &&  out_Buffer  &&  out_ulRets)  )
	 {
		 status=STATUS_INVALID_PARAMETER;
	 }else
	 {
		 pMethodBuffer=(ACPI_EVAL_INPUT_BUFFER_COMPLEX*)ExAllocatePool(PagedPool,ulMethodBufferSize);
		 if(!pMethodBuffer)
		 {
			 //KdPrint(("Out Of Memory   %s\n",__FUNCTION__));
		 }else
		 {
			 memset(pMethodBuffer,0,ulMethodBufferSize);
			 //memcpy_s(pMethodBuffer->MethodName,255,&MethodName,4);
			 //memcpy_s(pMethodBuffer->MethodName,255,"HPET._HID",9);
			 
			 pMethodBuffer->MethodNameAsUlong = MethodName;

			 pMethodBuffer->Size=ulMethodBufferSize;
			 pMethodBuffer->Signature=ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
			 pMethodBuffer->ArgumentCount=ArgumentCount;
			 memcpy(pMethodBuffer->Argument,in_Buffer,in_iBufferSize);	 
	/*if (ArgumentCount == 0)
	{
		KdPrint(("ACPI:ArgumentCount == 0"));
		pMethodBuffer->Signature=ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
		ulMethodBufferSize=sizeof(ACPI_EVAL_INPUT_BUFFER);
	}
	else if(ArgumentCount == 1 && pMethodBuffer->Argument[0].Type == 0)
	{
		KdPrint(("ACPI:ArgumentCount == 1 && pMethodBuffer->Argument[0].Type == 0"));
		pMethodBuffer->Signature=ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
		ulMethodBufferSize=sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER);
		((PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER)pMethodBuffer)->IntegerArgument = pMethodBuffer->Argument[0].Argument;
	 }*/
			//////////////////////////////////////////////////
			IO_STATUS_BLOCK     ioBlock={0};
			KEVENT              myIoctlEvent;
			PIRP                irp;
			KeInitializeEvent(&myIoctlEvent, SynchronizationEvent, FALSE);
			irp = IoBuildDeviceIoControlRequest(IOCTL_ACPI_EVAL_METHOD,Pdo,pMethodBuffer,ulMethodBufferSize,out_Buffer,in_oBufferSize,FALSE,&myIoctlEvent,&ioBlock);
			
			if (!irp)
			{
				status= STATUS_INSUFFICIENT_RESOURCES;
			}else
			{
				//irp->Tail.Overlay.CurrentStackLocation->DeviceObject = (PDEVICE_OBJECT)0x83BAB818;
				//IoGetNextIrpStackLocation(irp)->DeviceObject = (PDEVICE_OBJECT)0x83BAB818;
				pCurrentIrp = irp;
				//(irp->Tail.Overlay.CurrentStackLocation)->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_EVAL_METHOD_EX;
				DebugPrint(("ACPI: pCurrentIrp %08lx\n",pCurrentIrp));
				status = IoCallDriver(Pdo, irp);
				if (status == STATUS_PENDING)
				{
					DebugPrint(("ACPI: status == STATUS_PENDING\n"));
					KeWaitForSingleObject(&myIoctlEvent,Executive,KernelMode,FALSE,NULL);
					status = ioBlock.Status;
				}
				if(!ioBlock.Status)
				{
					*out_ulRets=ioBlock.Information;
				}else
				{
					*out_ulRets=0;
				}
			}
			 ExFreePool(pMethodBuffer);
		 }
	 }
	 return status;
}

ACPI_OBJECT* ReplaceObject(ACPI_OBJECT* in_hObject)
{
	ULONG pObject=0;
	if(acpihal_pdo)
	{
		
		pObject=*(ULONG*)((ULONG_PTR)(acpihal_pdo->DeviceExtension)+0x12c);
		//KdPrint(("ReplaceObject 0x%X to 0x%X\n",pObject,in_hObject));
		*(ULONG_PTR*)((ULONG_PTR)(acpihal_pdo->DeviceExtension)+0x12c)=(ULONG_PTR)in_hObject;
	}
	return (ACPI_OBJECT*)pObject;
}
VOID ntdmDrvIrpCancelRoutine(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	if(Irp->IoStatus.Status==STATUS_PENDING)
	{
		Irp->IoStatus.Status=STATUS_CANCELLED;
		Irp->IoStatus.Information=0;
		::IoCompleteRequest(Irp,IO_NO_INCREMENT);
		::IoReleaseCancelSpinLock(Irp->CancelIrql);
	}
}
NTSTATUS CreateDevice (IN PDRIVER_OBJECT	pDriverObject) 
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pdx;
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName,DEVICE_NAME);
	status = IoCreateDevice( pDriverObject,sizeof(DEVICE_EXTENSION),&(UNICODE_STRING)devName,FILE_DEVICE_ACPI,0, TRUE,&pDevObj );
	if (!NT_SUCCESS(status))
	{
		////KdPrint(("IoCreateDevice Failed EC=0x%lX\n",status));
	}else
	{
		////KdPrint(("IoCreateDevice OK\n"));
		pDevObj->Flags |= DO_BUFFERED_IO;
		pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		g_pdx=pdx;
		RtlZeroMemory(pdx,sizeof(DEVICE_EXTENSION));
		pdx->pDevice = pDevObj;
		pdx->ustrDeviceName = devName;
		UNICODE_STRING symLinkName;
		RtlInitUnicodeString(&symLinkName,SYMBOLINK_NAME);
		pdx->ustrSymLinkName = symLinkName;
		status = IoCreateSymbolicLink( &symLinkName,&devName );
		if (!NT_SUCCESS(status)) 
		{
			////KdPrint(("IoCreateSymbolicLink Failed EC=0x%lX\n",status));
			IoDeleteDevice( pDevObj );
		}else
		{
			////KdPrint(("IoCreateSymbolicLink OK\n"));
		}
	}
	return status;
}
