#include "EC.h"

void WriteCommand(UINT8 SMBAddr, UINT8 SMBData)
{
	while (_inp(EC_SC) & 0x02);
	_outp(EC_SC, 0x81);
	while (_inp(EC_SC) & 0x02);
	_outp(EC_DATA, SMBAddr);
	while (_inp(EC_SC) & 0x02);
	_outp(EC_DATA, SMBData);
}

UINT8 ReadCommand(UINT8 SMBAddr)
{
	UINT8 Data = 0;
	while (_inp(EC_SC) & 0x02);
	_outp(EC_SC, 0x80);
	while (_inp(EC_SC) & 0x02);
	_outp(EC_DATA, SMBAddr);
	while (!(_inp(EC_SC) & 0x01));
	Data = _inp(EC_DATA);

	return Data;
}

UINT8 PrintBatterySN(void)
{
	UINT8 Count, SMBDataBase = 0x1C;
	char SN[20] = { 0 };

	WriteCommand(0X19, 0X00);           //reset SMB_status
	WriteCommand(0x1A, 0x16);           //write SMB_Addr
	WriteCommand(0x1B, 0X20);           //SBS Command ManufacturerName
	WriteCommand(0X18, 0X0B);           //protocol
	while (ReadCommand(0X19) != 0x80);    //check status 
	Count = ReadCommand(0x3C);         //block count
	DebugPrint(("BOYCEHONG: the Count is %d\n", Count));

	for (int i = 0; i < Count; ++i)          // read
	{
		SN[i] = ReadCommand(SMBDataBase + i);
	}
	DebugPrint(("BOYCEHONG: %s\n", SN));

	return 0;
}

UINT8 QueryCommand(void)
{
	UINT8 Data = 0;

	//while (_inp(EC_SC) & 0x02); //No need to judge the SCI_EVT bit.
	//_outp(EC_SC, 0x84);
	//while (!(_inp(EC_SC) & 0x01));
	while (_inp(EC_SC) & 0x20);//Wait system to QueryEC command
	Data = _inp(EC_DATA);

	//DebugPrint(("BOYCEHONG: [1] _Q%02x\n", Data));

	return Data;
}