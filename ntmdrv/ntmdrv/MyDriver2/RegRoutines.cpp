#include "ntmdrv.h"
#pragma warning(disable:4311)
#pragma warning(disable:4302)

extern PDEVICE_EXTENSION g_pdx;
extern PKEVENT g_pApMethodEvent;
extern PIRP pCurrentIrp;

ULONG RegGetValue(WCHAR *in_wszKeyPath,ULONG in_ulType,ULONG in_ulIndex,PVOID *out_data)
{
	NTSTATUS status =STATUS_UNSUCCESSFUL;
	ULONG ulSize=0;
	UNICODE_STRING usRegString;
	OBJECT_ATTRIBUTES oa;
	HANDLE hRegister;
	RtlInitUnicodeString( &usRegString,in_wszKeyPath);
	InitializeObjectAttributes(&oa, &usRegString,OBJ_CASE_INSENSITIVE,NULL,NULL );
	status=ZwOpenKey( &hRegister,KEY_QUERY_VALUE,&oa);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwOpenKey Failed status=0x%lX in %s\n",status,__FUNCTION__));
	}else
	{
		ZwEnumerateValueKey(hRegister,in_ulIndex,(KEY_VALUE_INFORMATION_CLASS)in_ulType,0,0,&ulSize);
		ulSize+=10;
		UCHAR* pVerbi =(UCHAR*)ExAllocatePool(PagedPool,ulSize);
		if(!pVerbi)
		{
			status =STATUS_UNSUCCESSFUL;
			KdPrint(("ExAllocatePool Failed\n"));
		}
		{
			RtlSecureZeroMemory(pVerbi,ulSize);
			if(status=ZwEnumerateValueKey(hRegister,in_ulIndex,(KEY_VALUE_INFORMATION_CLASS)in_ulType,pVerbi,ulSize,&ulSize))
			{
				ExFreePool(pVerbi);
				KdPrint(("ZwEnumerateValueKey Failed status=0x%lX\n",status));
			}else
			{
				*out_data=(PVOID)pVerbi;
			}
			//ExFreePool(pVerbi);
		}
		ZwClose(hRegister);
	}
	return status;
}
ULONG RegGetKey(WCHAR *in_wszPath,ULONG in_ulType,ULONG in_ulIndex,PVOID* out_data)
{
	 
	NTSTATUS status =STATUS_UNSUCCESSFUL;
	ULONG ulSize=0;
	UNICODE_STRING usRegString;
	OBJECT_ATTRIBUTES oa;
	HANDLE hRegister;
	RtlInitUnicodeString( &usRegString,in_wszPath);
	InitializeObjectAttributes(&oa, &usRegString,OBJ_CASE_INSENSITIVE,NULL,NULL );
	status=ZwOpenKey( &hRegister,KEY_QUERY_VALUE,&oa);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwOpenKey Failed status=0x%lX in %s\n",status,__FUNCTION__));
	}else
	{
		ZwEnumerateKey(hRegister,in_ulIndex,(KEY_INFORMATION_CLASS)in_ulType ,NULL,0,&ulSize);
		ulSize+=10;
		UCHAR* pRootbi =(UCHAR*)ExAllocatePool(NonPagedPool,ulSize);
		if(!pRootbi)
		{
			status =STATUS_UNSUCCESSFUL;
			KdPrint(("ExAllocatePool Failed\n"));
		}else
		{
			RtlSecureZeroMemory(pRootbi,ulSize);
			if(status=ZwEnumerateKey(hRegister,in_ulIndex,(KEY_INFORMATION_CLASS)in_ulType ,pRootbi,ulSize,&ulSize))
			{
				ExFreePool(pRootbi);
				KdPrint(("ZwEnumerateValueKey Failed status=0x%lX\n",status));
			}else
			{
				*out_data=(PVOID)pRootbi;
			}
			ExFreePool(pRootbi);
		}
		ZwClose(hRegister);
	}
	return status;
}

HANDLE CreateRegKey(WCHAR *in_wszPath,WCHAR *in_wszName)
{
	HANDLE hRegister=0;
	if(in_wszPath  &&  in_wszName)
	{
		WCHAR *wszTemp=(WCHAR*)ExAllocatePool(PagedPool,(wcslen(in_wszPath)+wcslen(in_wszName)+4)*sizeof(WCHAR));
		if(wszTemp)
		{
			wcscpy(wszTemp,in_wszPath);
			wcscat(wszTemp,L"\\");
			wcscat(wszTemp,in_wszName);
			//////////////////////////
			OBJECT_ATTRIBUTES oa;
			ULONG ulResult=0;
			ULONG ulDisposition=0;
			UNICODE_STRING usRegString;
			RtlInitUnicodeString( &usRegString,wszTemp);
			InitializeObjectAttributes(&oa, &usRegString,OBJ_CASE_INSENSITIVE,NULL,NULL );
			ulResult=ZwCreateKey(&hRegister,KEY_ALL_ACCESS,&oa,0,0,REG_OPTION_VOLATILE,&ulDisposition);
			if(ulResult)
			{
				KdPrint(("ZwCreateKey Failed To Create %S EC=0x%X\n",wszTemp,ulResult));
			}
			ExFreePool(wszTemp);
		}
	}
	return hRegister;	
}
HANDLE OpenRegKey(WCHAR *in_wszPath,WCHAR *in_wszName)
{
	HANDLE hRegister=0;
	if(in_wszPath  &&  in_wszName)
	{
		WCHAR *wszTemp=(WCHAR*)ExAllocatePool(PagedPool,(wcslen(in_wszPath)+wcslen(in_wszName)+4)*sizeof(WCHAR));
		if(wszTemp)
		{
			wcscpy(wszTemp,in_wszPath);
			wcscat(wszTemp,L"\\");
			wcscat(wszTemp,in_wszName);
			//////////////////////////
			OBJECT_ATTRIBUTES oa;
			ULONG ulResult=0;
			ULONG ulDisposition=0;
			UNICODE_STRING usRegString;
			RtlInitUnicodeString( &usRegString,wszTemp);
			InitializeObjectAttributes(&oa, &usRegString,OBJ_CASE_INSENSITIVE,NULL,NULL );
			ulResult=ZwOpenKey(&hRegister,KEY_ALL_ACCESS,&oa);
			if(ulResult)
			{
				KdPrint(("ZwCreateKey Failed To Open %S EC=0x%X\n",wszTemp,ulResult));
			}
			ExFreePool(wszTemp);
		}
	}
	return hRegister;	
}

UCHAR CreateTraceData()
{
	UCHAR ucResult=0;
	HANDLE hReg=CreateRegKey(L"\\Registry\\Machine\\SOFTWARE",L"AcpiTool");
	if(hReg)
	{
		if(hReg)ZwClose(hReg);
		hReg=CreateRegKey(L"\\Registry\\Machine\\SOFTWARE\\AcpiTool",L"TraceData");
		if(hReg)
		{
			ucResult=1;
			if(hReg)ZwClose(hReg);
		}
	}
	return ucResult;
}
UCHAR DeleteTraceData()
{
			DebugPrint(("DeleteTraceData\n"));
	UCHAR ucResult=0;
	NTSTATUS status;
	HANDLE hReg=OpenRegKey(L"\\Registry\\Machine\\SOFTWARE\\AcpiTool",L"TraceData");
	if(hReg)
	{
		if((status = ZwDeleteKey(hReg)) == STATUS_SUCCESS)
		{
			ucResult = 1;
		}
		else
		{
			DebugPrint(("ACPI: ZwDeleteKey failed %08lx\n",status));
		}
		ZwClose(hReg);
	}
	hReg=OpenRegKey(L"\\Registry\\Machine\\SOFTWARE",L"AcpiTool");
	if(hReg)
	{
		if((status = ZwDeleteKey(hReg)) == STATUS_SUCCESS)
		{
			ucResult = 1;
		}
		else
		{
			DebugPrint(("ACPI: ZwDeleteKey failed %08lx\n",status));
		}
		ZwClose(hReg);
	}
	return ucResult;
}
void ULONG2WSZ(ULONG ulData,WCHAR *wszOutput)
{
	int i=0;
	for(i=0;i<sizeof(ULONG)*2;i++)
	{
		WCHAR wcData=( (  ulData<<(i*4) )&0xf0000000)>>28;
		if(wcData<0xa)
		{
			wcData+=(SHORT)L'0';
		}else
		{
			wcData-=0xa;
			wcData+=(SHORT)L'A';
		}
		wszOutput[i]=wcData;
	}
	wszOutput[i]=L'\0';
}
extern KSPIN_LOCK g_IrpSpinLock;
ULONG g_ulObjectArray[256]={0};
#define IRP_BUFFER_SIZE 1024
#define MEMORY_TAG 'mTgf'
ULONG g_ulTag=0;
HOOKED_DATA g_DataHeader={0};
KEVENT g_MtEvent;
KSPIN_LOCK g_LiskSpinLock;
ULONG g_ulSelfSynch=0;
void __cdecl  SystemMtThread(PVOID pvContext)
{
	UCHAR g_ucIrpBuffer[IRP_BUFFER_SIZE]={0};
	while(1)
	{
		KeWaitForSingleObject(&g_MtEvent,Executive,KernelMode ,0,0);
		//KdPrint(("SystemMtThread Signaled\n"));
		KeResetEvent(&g_MtEvent);
		if(!IsListEmpty((PLIST_ENTRY)&g_DataHeader))
		{
			HOOKED_DATA *pEntry=(HOOKED_DATA*)g_DataHeader.list.Flink;
			HOOKED_DATA *pTemp=0;
			while(pEntry!=&g_DataHeader)
			{
				if(pEntry->ucDealed==1)
				{
					WCHAR wszDatas[20]={0};
					ULONG2WSZ((ULONG)g_ulTag++,wszDatas);
					HANDLE hReg=CreateRegKey(L"\\Registry\\Machine\\SOFTWARE\\AcpiTool\\TraceData",wszDatas);
					if(hReg)
					{
						UNICODE_STRING usTemp;
						//////////////////////////////////
						ULONG ulRet=0;
						//////////////////////////////////////////
						RtlSecureZeroMemory(g_ucIrpBuffer,sizeof(WCHAR)*2);

						//DebugPrint(("ACPI: pEntry->DeviceObject %08lx\n",pEntry->DeviceObject));
						//DebugPrint(("ACPI: pEntry->irp %08lx\n",pEntry->irp));
						if(pEntry->DeviceObject != NULL)
						{
							if((pEntry->DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE )!= 0)
							{
								DebugPrint(("ACPI: [BOYCEHONG] SystemMtThread DO_BUS_ENUMERATED_DEVICE \n"));
								RtlSecureZeroMemory(g_ucIrpBuffer,sizeof(WCHAR)*2);
								IoGetDeviceProperty(pEntry->DeviceObject,DevicePropertyDeviceDescription,IRP_BUFFER_SIZE,g_ucIrpBuffer,&ulRet);
								RtlInitUnicodeString(&usTemp,L"Description");
								ZwSetValueKey(hReg,&usTemp,0,REG_NONE,g_ucIrpBuffer,ulRet);
								
								//////////////////////////////////////////////
								RtlSecureZeroMemory(g_ucIrpBuffer,sizeof(WCHAR)*2);
								IoGetDeviceProperty(pEntry->DeviceObject,DevicePropertyHardwareID ,IRP_BUFFER_SIZE,g_ucIrpBuffer,&ulRet);
								RtlInitUnicodeString(&usTemp,L"HwID");
								ZwSetValueKey(hReg,&usTemp,0,REG_NONE,g_ucIrpBuffer,ulRet);
							}
							else
							{
								DebugPrint(("ACPI: is not DO_BUS_ENUMERATED_DEVICE \n"));
								DebugPrint(("ACPI: pCurrentIrp %08lx\n",pCurrentIrp));
								DebugPrint(("ACPI: pEntry->irp %08lx\n",pEntry->irp));
								if(pEntry->irp == pCurrentIrp)
								{
									RtlInitUnicodeString(&usTemp,L"Description");
									ZwSetValueKey(hReg,&usTemp,0,REG_NONE,L"AcpiTool",18);
								}
							}
						}
						else
						{
							DebugPrint(("ACPI: pEntry->DeviceObject is NULL\n"));
						}
						if (pCurrentIrp != NULL)
						{
							pCurrentIrp = NULL;
						}
						////////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"DeviceObject");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->DeviceObject,sizeof(PVOID));
						//////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"irp");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->irp,sizeof(PVOID));
						//////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"iTime");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->time_in,sizeof(TIME_FIELDS));
						//////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"MethodPath");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)],pEntry->ulPathSize);
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"InputSize");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->ulInputSize,sizeof(ULONG));
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"ControlCode");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->ulControlCode,sizeof(ULONG));
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"OutputSize");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->ulOutputSize,sizeof(ULONG));
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"InputData");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)+pEntry->ulPathSize],pEntry->ulInputSize);
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"oTime");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->time_out,sizeof(TIME_FIELDS));
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"Information");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->ulInformation,sizeof(ULONG));
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"Status");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&pEntry->ulStatus,sizeof(ULONG));
						////////////////////////////////////////
						RtlInitUnicodeString(&usTemp,L"OutputData");
						ZwSetValueKey(hReg,&usTemp,0,REG_NONE,&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)+pEntry->ulPathSize+pEntry->ulInputSize],pEntry->ulOutputSize);
						////////////////////////////////////////
						ZwClose(hReg);  
					}
					KIRQL irql;
					pTemp=pEntry;
					pEntry=(HOOKED_DATA *)pEntry->list.Flink;	
					if(pTemp!=&g_DataHeader)
					{
						KdPrint(("SystemMtThread 1 pTemp=0x%X,pEntry=0x%x,pTemp->ulAddr=%X,PDO=0x%X,hThread=0x%X,curThread=0x%X\n",pTemp,pEntry,pTemp->ulAddr,pTemp->DeviceObject,pTemp->hThread,PsGetCurrentThreadId()));
						KeAcquireSpinLock(&g_LiskSpinLock,&irql);
						RemoveEntryList((PLIST_ENTRY)pTemp);	
						KeReleaseSpinLock(&g_LiskSpinLock,irql);
						KdPrint(("SystemMtThread 2 pTemp=0x%X,pEntry=0x%x,pTemp->ulAddr=%X\n",pTemp,pEntry,pTemp->ulAddr));
						if(!MmIsAddressValid(pTemp))
						{
							KdPrint(("pTemp=0x%X Can Not Be Freed\n",pTemp));
						}else
						{
							ExFreePoolWithTag(pTemp,MEMORY_TAG);
						}
						
					}
				}else pEntry=(HOOKED_DATA *)pEntry->list.Flink;							
			}
		}
		if(g_pApMethodEvent)
		{
			DebugPrint(("ACPI: set methodEvent"));
			KeSetEvent(g_pApMethodEvent,0,0);
		}
	}
}
void DealTrace(PDEVICE_OBJECT in_pdo,PIRP in_irp,UCHAR ucIsInput)
{
	//DebugPrint(("ACPI: DealTrace\n"));
	//DebugPrint(("ACPI: in_pdo %08lx, in_irp %08lx\n",in_pdo,in_irp));
	if(/*!in_pdo  ||*/  !in_irp)
		{
			return ;
	}
	ULONG ulSize=0;
	ULONG ulOutputBufferSize=0;
	ULONG ulInputBufferSize=0;
	PIO_STACK_LOCATION stack =0;
	LARGE_INTEGER liTime,liLocTime;
	KeQuerySystemTime(&liTime);
	ExSystemTimeToLocalTime(&liTime,&liLocTime);
	if(ucIsInput)
		stack=IoGetCurrentIrpStackLocation(in_irp);
	else
		stack =IoGetCurrentIrpStackLocation(in_irp);
	if(!stack)
	{
		//DebugPrint(("ACPI: stack if NULL\n"));
		return;
	}
	//DebugPrint(("ACPI: stack %08lx\n",stack));
	ulInputBufferSize=stack->Parameters.DeviceIoControl.InputBufferLength;
	ulOutputBufferSize=stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG dwIoControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	//DebugPrint(("ACPI: ulInputBufferSize %d\n",ulInputBufferSize));
	//DebugPrint(("ACPI: ulOutputBufferSize %d\n",ulOutputBufferSize));
	//DebugPrint(("ACPI: dwIoControlCode %08lx\n",dwIoControlCode));
	//DebugPrint(("ACPI: ucIsInput %d\n",ucIsInput));
	if(ucIsInput)
	{//Input
		//////////////////////////////////////////////////////////////////////
		ULONG ulPos=255;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX pInputBuffer;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX pInputBufferEx;
		LONG nameLength;
		//DebugPrint(("ACPI: InputBufferLength %d\n",IoGetCurrentIrpStackLocation(in_irp)->Parameters.DeviceIoControl.InputBufferLength));
		switch (IoGetCurrentIrpStackLocation(in_irp)->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_ACPI_ASYNC_EVAL_METHOD:
		case IOCTL_ACPI_EVAL_METHOD:
			pInputBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)in_irp->AssociatedIrp.SystemBuffer;
			g_ulObjectArray[ulPos]=pInputBuffer->MethodNameAsUlong;
			break;
		case IOCTL_ACPI_EVAL_METHOD_EX:
		case IOCTL_ACPI_ASYNC_EVAL_METHOD_EX:
			pInputBufferEx = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)in_irp->AssociatedIrp.SystemBuffer;
			nameLength = strnlen_s(pInputBufferEx->MethodName,255);
			while(nameLength-(LONG)(5*(255-ulPos))-4 >= 0 && ulPos>0)
			{
				memcpy_s(&(g_ulObjectArray[ulPos]),4,&(pInputBufferEx->MethodName[nameLength-5*(255-ulPos)-4]),4);
				ulPos --;
			}
			ulPos++;
			break;
		default:
			g_ulObjectArray[ulPos]=((ULONG*)in_irp->AssociatedIrp.SystemBuffer)[1];
			break;
		}
		ACPI_OBJECT *acpiobj=(ACPI_OBJECT*)GetAcpiHandle(in_pdo);
		while(acpiobj  && ulPos>0)
		{
			g_ulObjectArray[--ulPos]=acpiobj->ulObject;
			acpiobj=acpiobj->pParent;
		}
		ulSize=ulInputBufferSize+ulOutputBufferSize+sizeof(HOOKED_DATA)+(256-ulPos)*sizeof(ULONG);
		KIRQL irql=KeRaiseIrqlToDpcLevel();
		HOOKED_DATA *pEntry=(HOOKED_DATA*)ExAllocatePoolWithTag(NonPagedPool,ulSize,MEMORY_TAG);
		if(pEntry)
		{
			InitializeListHead((PLIST_ENTRY)pEntry);
			pEntry->ulAddr= (ULONG)pEntry;
			pEntry->irql=KeGetCurrentIrql();
			pEntry->DeviceObject=in_pdo;
			pEntry->hThread=PsGetCurrentThreadId();
			pEntry->irp=in_irp;
			pEntry->ucDealed=0;
			pEntry->ulControlCode=dwIoControlCode;
			RtlTimeToTimeFields(&liLocTime,&pEntry->time_in);
			pEntry->ulInputSize=ulInputBufferSize;
			pEntry->ulOutputSize=ulOutputBufferSize;
			pEntry->ulPathSize=(256-ulPos)*sizeof(ULONG);
			//memcpy(&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)],&g_ulObjectArray[ulPos],pEntry->ulPathSize);
			RtlMoveMemory(&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)],&g_ulObjectArray[ulPos],pEntry->ulPathSize);
			if(ulInputBufferSize)
				//memcpy(&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)+pEntry->ulPathSize],in_irp->AssociatedIrp.SystemBuffer,ulInputBufferSize);
				RtlMoveMemory(&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)+pEntry->ulPathSize],in_irp->AssociatedIrp.SystemBuffer,ulInputBufferSize);
			DebugPrint(("ACPI: [BOYCEHONG] ExInterlockedInsertHeadList\n"));
			ExInterlockedInsertHeadList((PLIST_ENTRY)&g_DataHeader,(PLIST_ENTRY)pEntry,&g_LiskSpinLock);
		}
		KeLowerIrql(irql);
	}else
	{//Output
		HOOKED_DATA *pEntry=(HOOKED_DATA*)g_DataHeader.list.Flink;
		while(pEntry!=&g_DataHeader)
		{
			if(/*pEntry->DeviceObject==in_pdo  &&*/  pEntry->irp==in_irp  &&  in_irp->IoStatus.Status!=STATUS_PENDING)
			{
				pEntry->ulInformation=in_irp->IoStatus.Information;
				pEntry->ulStatus=in_irp->IoStatus.Status;
				pEntry->ucDealed=1;
				RtlTimeToTimeFields(&liLocTime,&pEntry->time_out);
				if(ulOutputBufferSize)
					//memcpy(&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)+pEntry->ulPathSize+pEntry->ulInputSize],in_irp->AssociatedIrp.SystemBuffer,pEntry->ulOutputSize);
					RtlMoveMemory(&((PUCHAR)pEntry)[sizeof(HOOKED_DATA)+pEntry->ulPathSize+pEntry->ulInputSize],in_irp->AssociatedIrp.SystemBuffer,pEntry->ulInformation);
				KeSetEvent(&g_MtEvent,0,0);
				break;
			}
			pEntry=(HOOKED_DATA *)pEntry->list.Flink;
		}
		//
	}
}